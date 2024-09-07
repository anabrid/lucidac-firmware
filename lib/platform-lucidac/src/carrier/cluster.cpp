// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier/cluster.h"
#include "bus/bus.h"
#include "utils/logging.h"
#include "utils/running_avg.h"

#include "block/cblock.h"
#include "block/iblock.h"
#include "block/mblock.h"
#include "block/ublock.h"

std::array<blocks::FunctionBlock *, 6> platform::Cluster::get_blocks() const {
  return {m0block, m1block, ublock, cblock, iblock, shblock};
}

bool platform::Cluster::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);

  // Dynamically detect installed blocks
  // Check if a block is already set, which may happen with a special constructor in the future
  LOG(ANABRID_DEBUG_INIT, "Detecting installed blocks...");
  if (!m0block) {
    m0block = entities::detect<blocks::MBlock>(bus::idx_to_addr(cluster_idx, bus::M0_BLOCK_IDX, 0));
    if (!m0block)
      LOG(ANABRID_DEBUG_INIT, "Warning: M0-block is missing or unknown.");
  }
  if (!m1block) {
    m1block = entities::detect<blocks::MBlock>(bus::idx_to_addr(cluster_idx, bus::M1_BLOCK_IDX, 0));
    if (!m1block)
      LOG(ANABRID_DEBUG_INIT, "Warning: M1-block is missing or unknown.");
  }
  if (!m0block and !m1block)
    LOG(ANABRID_DEBUG_INIT, "Error: Both M0 and M1-blocks are missing or unknown.");

  if (!ublock) {
    ublock = entities::detect<blocks::UBlock>(bus::idx_to_addr(cluster_idx, bus::U_BLOCK_IDX, 0));
    if (!ublock)
      LOG_ERROR("Error: U-block is missing or unknown.");
  }
  if (!cblock) {
    cblock = entities::detect<blocks::CBlock>(bus::idx_to_addr(cluster_idx, bus::C_BLOCK_IDX, 0));
    if (!cblock)
      LOG_ERROR("Error: C-block is missing or unknown.");
  }
  if (!iblock) {
    iblock = entities::detect<blocks::IBlock>(bus::idx_to_addr(cluster_idx, bus::I_BLOCK_IDX, 0));
    if (!iblock)
      LOG_ERROR("Error: I-block is missing or unknown.");
  }
  if (!shblock) {
    shblock = entities::detect<blocks::SHBlock>(bus::idx_to_addr(cluster_idx, bus::SH_BLOCK_IDX, 0));
    if (!shblock)
      LOG_ERROR("Error: SH-block is missing or unknown.");
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
  if (!ublock or !shblock)
    return false; // Fatal error preventing any further regular operation in the system

  auto old_transmission_modes = ublock->get_all_transmission_modes();

  ublock->change_all_transmission_modes(blocks::UBlock::Transmission_Mode::GROUND);
  if (!ublock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  delay(10);
  shblock->compensate_hardware_offsets();

  ublock->change_all_transmission_modes(old_transmission_modes);
  if (!ublock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  return true;
}

bool platform::Cluster::calibrate_routes(daq::BaseDAQ *daq) {
  bool success = true;
  // CARE: This function assumes that certain preparations have been made, see Carrier::calibrate.

  // Save current U-block transmission modes and set them to zero
  LOG_ANABRID_DEBUG_CALIBRATION("Starting calibration");
  auto old_transmission_modes = ublock->get_all_transmission_modes();
  auto old_reference_magnitude = ublock->get_reference_magnitude();

  // Save C-Block factors
  auto old_c_block_factors = cblock->get_factors();
  cblock->set_factors({});

  // Actually write to hardware
  LOG_ANABRID_DEBUG_CALIBRATION("Reset c-block");
  if (!cblock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  auto old_i_block_upscaling = iblock->get_upscales();
  iblock->reset_upscaling();
  if (!iblock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  // Next, we iterate through all connections on the I-block and calibrate each lane individually.
  // This is suboptimal, as in principle we could measure 8 gain corrections at the same time.
  // Iterating over I-block connections is easier than iterating over U-block outputs.
  for (auto i_out_idx : blocks::IBlock::OUTPUT_IDX_RANGE()) {
    for (auto i_in_idx : blocks::IBlock::INPUT_IDX_RANGE()) {
      // Only do something is this lane is used in the original I-block configuration
      if (!iblock->is_connected(i_in_idx, i_out_idx))
        continue;

      // If we don't have a complete connection through the u block, we don't need / we can't calibrate this
      // route from here
      if (!ublock->is_output_connected(i_in_idx))
        continue;

      LOG_ANABRID_DEBUG_CALIBRATION("Calibrating connection: ");
      LOG_ANABRID_DEBUG_CALIBRATION(i_in_idx);
      LOG_ANABRID_DEBUG_CALIBRATION(i_out_idx);

      // Depending on whether upscaling is enabled for this lane, we apply +1 or +0.1 reference
      // This is done on all lanes (but no other I-block connection exists, so no other current flows)
      bool upscaled_channel = iblock->get_upscaling(i_in_idx);
      ublock->change_all_transmission_modes(blocks::UBlock::Transmission_Mode::POS_REF);
      ublock->change_reference_magnitude(upscaled_channel ? blocks::UBlock::Reference_Magnitude::ONE_TENTH
                                                          : blocks::UBlock::Reference_Magnitude::ONE);
      // Actually write to hardware
      if (!ublock->write_to_hardware())
        return false; // Fatal error preventing any further regular operation in the system

      // Allow this connection to go up to full scale. Those values are allways legal so we can ignore the
      // return value
      (void)cblock->set_factor(i_in_idx, 1.0f);
      (void)cblock->set_gain_correction(i_in_idx, 1.0f);
      // Actually write to hardware
      if (!cblock->write_to_hardware())
        return false; // Fatal error preventing any further regular operation in the system

      // Calibrate offsets for this specific route
      if (!calibrate_offsets())
        return false; // Fatal error preventing any further regular operation in the system

      // Change SH-block into gain mode and select correct gain channel group
      if (i_out_idx < 8)
        shblock->set_state(blocks::SHBlock::State::GAIN_ZERO_TO_SEVEN);
      else
        shblock->set_state(blocks::SHBlock::State::GAIN_EIGHT_TO_FIFTEEN);
      if (!shblock->write_to_hardware())
        return false; // Fatal error preventing any further regular operation in the system

      // Chill for a bit
      delay(10);

      // Measure gain output
      auto m_adc = daq->sample_avg(4, 10)[i_out_idx % 8];
      LOG_ANABRID_DEBUG_CALIBRATION(m_adc);
      // Calculate necessary gain correction
      auto gain_correction = 1.0f / m_adc;
      LOG_ANABRID_DEBUG_CALIBRATION(gain_correction);
      // Set gain correction on C-block, which will automatically get applied when writing to hardware
      if (!cblock->set_gain_correction(i_in_idx, gain_correction)) {
        LOG_ANABRID_DEBUG_CALIBRATION("Gain correction could not be set as it is out of range. Resetting this "
                                      "channel to default corretion");
        (void)cblock->set_gain_correction(i_in_idx, 1.0f);
        success = false;
      }

      // Deactivate this lane again
      (void)cblock->set_factor(i_in_idx, 0.0f);
      if (!cblock->write_to_hardware()) // This write_to_hardware could be left out, but it's a nice safety
                                        // measure
        return false;                   // Fatal error preventing any further regular operation in the system
      LOG_ANABRID_DEBUG_CALIBRATION(" ");
    }
  }

  // Restore C-block factors
  cblock->set_factors(old_c_block_factors);
  LOG_ANABRID_DEBUG_CALIBRATION("Restoring c-block");
  if (!cblock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  // Calibrate offsets again, since they have been changed by correcting the coefficients
  if (!calibrate_offsets())
    return false; // Fatal error preventing any further regular operation in the system

  // Restore original U-block transmission modes and reference
  ublock->change_all_transmission_modes(old_transmission_modes);
  ublock->change_reference_magnitude(old_reference_magnitude);
  // Write them to hardware
  LOG_ANABRID_DEBUG_CALIBRATION("Restoring u-block");
  if (!ublock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  iblock->set_upscaling(old_i_block_upscaling);
  if (!iblock->write_to_hardware()) {
    return false; // Fatal error preventing any further regular operation in the system
  }

  // Switch SHBlock into inject mode
  shblock->set_state(blocks::SHBlock::State::INJECT);
  if (!shblock->write_to_hardware())
    return false; // Fatal error preventing any further regular operation in the system

  return success;
}

bool platform::Cluster::calibrate_m_blocks(daq::BaseDAQ *daq) {
  bool success = true;
  for (auto mblock : {m0block, m1block}) {
    if (mblock)
      if (!mblock->calibrate(daq, this))
        success = false;
  }
  return success;
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

bool platform::Cluster::route_in_external(uint8_t in, uint8_t i_out) {
  if (in > 7)
    return false;

  return iblock->connect(in + 24, i_out);
}

bool platform::Cluster::route_out_external(uint8_t u_in, uint8_t out, float c_factor) {
  if (out > 7)
    return false;

  if (!ublock->connect(u_in, out + 24))
    return false;
  return cblock->set_factor(out + 24, c_factor);
}

void platform::Cluster::reset(bool keep_calibration) {
  for (auto block : get_blocks()) {
    if (block)
      block->reset(keep_calibration);
  }
}

entities::Entity *platform::Cluster::get_child_entity(const std::string &child_id) {
  if (child_id == "M0")
    return m0block;
  else if (child_id == "M1")
    return m1block;
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

utils::status platform::Cluster::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  // Cluster has no own configuration parameters currently
  // TODO: Have an option to fail on unexpected configuration
  return utils::status::success();
}

std::vector<entities::Entity *> platform::Cluster::get_child_entities() {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  return {m0block, m1block, ublock, cblock, iblock, shblock};
}

uint8_t platform::Cluster::get_cluster_idx() const { return cluster_idx; }
