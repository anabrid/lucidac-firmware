// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "cluster.h"
#include "bus/bus.h"
#include "logging.h"
#include "running_avg.h"

#define RETURN_FALSE_IF_FAILED(x)                                                                             \
  if (!(x))                                                                                                   \
    return false;

std::array<blocks::FunctionBlock *, 6> platform::Cluster::get_blocks() const {
  return {m1block, m2block, ublock, cblock, iblock, shblock};
}

bool platform::Cluster::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);

  // Dynamically detect installed blocks
  // Check if a block is already set, which may happen with a special constructor in the future
  LOG(ANABRID_DEBUG_INIT, "Detecting installed blocks...");
  if (!m1block) {
    m1block = entities::detect<blocks::MBlock>(bus::idx_to_addr(cluster_idx, bus::M1_BLOCK_IDX, 0));
    if (!m1block)
      LOG(ANABRID_DEBUG_INIT, "Warning: M0-block is missing or unknown.");
  }
  if (!m2block) {
    m2block = entities::detect<blocks::MBlock>(bus::idx_to_addr(cluster_idx, bus::M2_BLOCK_IDX, 0));
    if (!m2block)
      LOG(ANABRID_DEBUG_INIT, "Warning: M1-block is missing or unknown.");
  }
  if (!m1block and !m2block)
    LOG(ANABRID_DEBUG_INIT, "Error: Both M0 and M1-blocks are missing or unknown.");

  if (!ublock) {
    ublock = entities::detect<blocks::UBlock>(bus::idx_to_addr(cluster_idx, bus::U_BLOCK_IDX, 0));
    if (!ublock)
      LOG(ANABRID_DEBUG_INIT, "Error: U-block is missing or unknown.");
  }
  if (!cblock) {
    cblock = entities::detect<blocks::CBlock>(bus::idx_to_addr(cluster_idx, bus::C_BLOCK_IDX, 0));
    if (!cblock)
      LOG(ANABRID_DEBUG_INIT, "Error: C-block is missing or unknown.");
  }
  if (!iblock) {
    iblock = entities::detect<blocks::IBlock>(bus::idx_to_addr(cluster_idx, bus::I_BLOCK_IDX, 0));
    if (!iblock)
      LOG(ANABRID_DEBUG_INIT, "Error: I-block is missing or unknown.");
  }
  if (!shblock) {
    shblock = entities::detect<blocks::SHBlock>(bus::idx_to_addr(cluster_idx, bus::SH_BLOCK_IDX, 0));
    if (!shblock)
      LOG(ANABRID_DEBUG_INIT, "Error: SH-block is missing or unknown.");
  }

  LOG(ANABRID_DEBUG_INIT, "Initialising detected blocks...");
  for (auto block : get_blocks()) {
    if (block && !block->init())
      return false;
  }
  LOG(ANABRID_DEBUG_INIT, "Cluster initialized.");
  reset(false);
  return true;
}

platform::Cluster::Cluster(uint8_t cluster_idx)
    : entities::Entity(std::to_string(cluster_idx)), cluster_idx(cluster_idx) {}

bool platform::Cluster::calibrate_offsets() {
  LOG_ANABRID_DEBUG_CALIBRATION("Calibrating offsets");

  auto old_transmission_modes = ublock->get_all_transmission_modes();

  ublock->change_all_transmission_modes(blocks::UBlock::Transmission_Mode::GROUND);
  if (!ublock->write_to_hardware())
    return false;

  shblock->compensate_hardware_offsets();

  ublock->change_all_transmission_modes(old_transmission_modes);
  if (!ublock->write_to_hardware())
    return false;

  return true;
}

bool platform::Cluster::calibrate(daq::BaseDAQ *daq) {
  // CARE: This function assumes that certain preparations have been made, see Carrier::calibrate.

  // Save current U-block transmission modes and set them to zero
  LOG_ANABRID_DEBUG_CALIBRATION("Starting calibration");
  auto old_transmission_modes = ublock->get_all_transmission_modes();
  auto old_reference_magnitude = ublock->get_reference_magnitude();
  ublock->change_all_transmission_modes(blocks::UBlock::Transmission_Mode::GROUND);
  // Save and reset I-block connections
  // This is necessary as we can not calibrate sums and we want to calibrate lanes individually
  const auto saved_iblock = *iblock;
  iblock->reset_outputs();
  // Actually write to hardware
  LOG_ANABRID_DEBUG_CALIBRATION("Reset ublock and i block");
  if (!ublock->write_to_hardware() and iblock->write_to_hardware()) {
    // TODO: Restore configuration in this and other failure cases
    return false;
  }

  // Next, we iterate through all connections on the I-block and calibrate each lane individually.
  // This is suboptimal, as in principle we could measure 8 gain corrections at the same time.
  // Iterating over I-block connections is easier than iterating over U-block outputs.
  for (auto i_out_idx : blocks::IBlock::OUTPUT_IDX_RANGE()) {
    for (auto i_in_idx : blocks::IBlock::INPUT_IDX_RANGE()) {
      // Only do something is this lane is used in the original I-block configuration
      if (!saved_iblock.is_connected(i_in_idx, i_out_idx))
        continue;
      // Only do something is this lane is actually receiving a signal
      if (!ublock->is_output_connected(i_in_idx))
        continue;

      // Restore this connection
      if (!iblock->connect(i_in_idx, i_out_idx))
        return false;
      // Actually write to hardware
      if (!iblock->write_to_hardware())
        return false;

      // Chill for a bit
      delay(100);
      LOG_ANABRID_DEBUG_CALIBRATION("Calibrating connection: ");
      LOG_ANABRID_DEBUG_CALIBRATION(i_in_idx);
      LOG_ANABRID_DEBUG_CALIBRATION(i_out_idx);

      // First step is always to calibrate offsets
      calibrate_offsets();

      // Depending on whether upscaling is enabled for this lane, we apply +1 or +0.1 reference
      // This is done on all lanes (but no other I-block connection exists, so no other current flows)
      ublock->change_all_transmission_modes(blocks::UBlock::Transmission_Mode::POS_REF);
      if (!iblock->get_upscaling(i_in_idx))
        ublock->change_reference_magnitude(blocks::UBlock::Reference_Magnitude::ONE);
      else
        ublock->change_reference_magnitude(blocks::UBlock::Reference_Magnitude::ONE_TENTH);
      // Actually write to hardware
      if (!ublock->write_to_hardware())
        return false;

      // Change SH-block into gain mode and select correct gain channel group
      shblock->set_gain.trigger();
      if (i_out_idx < 8)
        shblock->set_gain_channels_zero_to_seven.trigger();
      else
        shblock->set_gain_channels_eight_to_fifteen.trigger();

      // Chill for a bit
      delay(100);

      // Measure gain output
      auto m_adc = daq->sample_avg(10, 1000)[i_out_idx % 8];
      LOG_ANABRID_DEBUG_CALIBRATION(m_adc);
      // Calculate necessary gain correction
      auto wanted_factor = cblock->get_factor(i_in_idx);
      auto gain_correction = wanted_factor / m_adc;
      LOG_ANABRID_DEBUG_CALIBRATION(gain_correction);
      // Set gain correction on C-block, which will automatically get applied when writing to hardware
      if (!cblock->set_gain_correction(i_in_idx, gain_correction))
        return false;

      // Disconnect this lane again
      iblock->disconnect(i_in_idx, i_out_idx);
      if (!iblock->write_to_hardware())
        return false;
      LOG_ANABRID_DEBUG_CALIBRATION(" ");
    }
  }

  // For "safety", set all U-block inputs to zero before restoring I-block connections
  LOG_ANABRID_DEBUG_CALIBRATION("Setting ublock to ground for safety");
  ublock->change_all_transmission_modes(blocks::UBlock::Transmission_Mode::GROUND);
  if (!ublock->write_to_hardware())
    return false;
  // Restore I-block connections
  iblock->set_outputs(saved_iblock.get_outputs());
  LOG_ANABRID_DEBUG_CALIBRATION("Restoring iblock");
  if (!iblock->write_to_hardware())
    return false;

  // Write C-block to hardware to apply gain corrections
  if (!cblock->write_to_hardware())
    return false;

  // Calibrate offsets again, since they have been changed by correcting the coefficients
  if (!calibrate_offsets())
    return false;

  // Restore original U-block transmission modes and reference
  ublock->change_all_transmission_modes(old_transmission_modes);
  ublock->change_reference_magnitude(old_reference_magnitude);
  // Write them to hardware
  LOG_ANABRID_DEBUG_CALIBRATION("Restoring ublock");
  if (!ublock->write_to_hardware())
    return false;

  return true;
}

bool platform::Cluster::write_to_hardware() {
  for (auto block : get_blocks()) {
    if (block)
      if (!block->write_to_hardware()) {
        LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
        return false;
      }
  }
  return true;
}

bool platform::Cluster::route(uint8_t u_in, uint8_t u_out, float c_factor, uint8_t i_out) {
  if (fabs(c_factor) > 1.0f) {
    c_factor = c_factor * 0.1f;
    iblock->set_upscaling(u_out, true);
  } else
    iblock->set_upscaling(u_out, false);

  if (!ublock->connect(u_in, u_out))
    return false;
  if (!cblock->set_factor(u_out, c_factor))
    return false;
  if (!iblock->connect(u_out, i_out))
    return false;
  return true;
}

bool platform::Cluster::add_constant(blocks::UBlock::Transmission_Mode signal_type, uint8_t u_out,
                                     float c_factor, uint8_t i_out) {
  if (fabs(c_factor) > 1.0f) {
    c_factor = c_factor * 0.1f;
    iblock->set_upscaling(u_out, true);
  } else
    iblock->set_upscaling(u_out, false);

  if (!ublock->connect_alternative(signal_type, u_out))
    return false;
  if (!cblock->set_factor(u_out, c_factor))
    return false;
  if (!iblock->connect(u_out, i_out))
    return false;
  return true;
}

void platform::Cluster::reset(bool keep_calibration) {
  for (auto block : get_blocks()) {
    if (block)
      block->reset(keep_calibration);
  }
}

entities::Entity *platform::Cluster::get_child_entity(const std::string &child_id) {
  if (child_id == "M0")
    return m1block;
  else if (child_id == "M1")
    return m2block;
  else if (child_id == "U")
    return ublock;
  else if (child_id == "C")
    return cblock;
  else if (child_id == "I")
    return iblock;
  else if (child_id == "SH")
    return shblock;
  return nullptr;
}

bool platform::Cluster::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  // Cluster has no own configuration parameters currently
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

std::vector<entities::Entity *> platform::Cluster::get_child_entities() {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  return {m1block, m2block, ublock, cblock, iblock, shblock};
}

uint8_t platform::Cluster::get_cluster_idx() const { return cluster_idx; }
