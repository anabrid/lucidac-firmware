// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier/cluster.h"
#include "bus/bus.h"
#include "utils/logging.h"
#include "utils/running_avg.h"

#define RETURN_FALSE_IF_FAILED(x)                                                                             \
  if (!(x))                                                                                                   \
    return false;

auto lucidac::LUCIDAC::get_blocks() {
  return std::array<blocks::FunctionBlock *, 5>{m1block, m2block, ublock, cblock, iblock};
}

bool lucidac::LUCIDAC::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  bus::init();
  for (auto block : get_blocks()) {
    if (block && !block->init())
      return false;
  }
  LOG(ANABRID_DEBUG_INIT, "Cluster initialized.");
  reset(false);
  return true;
}

lucidac::LUCIDAC::LUCIDAC(uint8_t cluster_idx)
    : entities::Entity(std::to_string(cluster_idx)), m1block{new blocks::MIntBlock{cluster_idx,
                                                                                   blocks::MBlock::M1_IDX}},
      m2block{new blocks::MMulBlock{cluster_idx, blocks::MBlock::M2_IDX}}, ublock{new blocks::UBlock{
                                                                               cluster_idx}},
      cblock{new blocks::CBlock{cluster_idx}}, iblock{new blocks::IBlock{cluster_idx}} {
  // TODO: Check for existence of blocks here instead of initializing them without checking
}

bool lucidac::LUCIDAC::calibrate(daq::BaseDAQ *daq) {
  RETURN_FALSE_IF_FAILED(calibrate_offsets_ublock_initial(daq));
  // Return to a non-connected state, but keep calibrated offsets
  reset(true);
  write_to_hardware();
  return true;
}

bool lucidac::LUCIDAC::calibrate_offsets_ublock_initial(daq::BaseDAQ *daq) {
  // Reset
  ublock->reset(false);
  // Connect REF signal from UBlock to ADC outputs
  ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF);
  for (auto out_to_adc : std::array<uint8_t, 8>{0, 1, 2, 3, 4, 5, 6, 7}) {
    ublock->connect(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, out_to_adc);
  }
  ublock->write_to_hardware();
  // Let the signal settle
  delayMicroseconds(250);

  auto data_avg = daq->sample_avg(10, 10000);
  for (size_t i = 0; i < data_avg.size(); i++) {
    RETURN_FALSE_IF_FAILED(ublock->change_offset(i, data_avg[i] + 1.0f));
  }
  ublock->write_to_hardware();
  delayMicroseconds(100);

  return true;
  // TODO: Finish calibration sequence
}

void lucidac::LUCIDAC::write_to_hardware() {
  for (auto block : get_blocks()) {
    if (block)
      block->write_to_hardware();
  }
}

bool lucidac::LUCIDAC::route(uint8_t u_in, uint8_t u_out, float c_factor, uint8_t i_out) {
  if (!ublock->connect(u_in, u_out))
    return false;
  if (!cblock->set_factor(u_out, c_factor))
    return false;
  if (!iblock->connect(u_out, i_out))
    return false;
  return true;
}

bool lucidac::LUCIDAC::route_alt_signal(uint16_t alt_signal, uint8_t u_out, float c_factor, uint8_t i_out) {
  if (!ublock->connect_alt_signal(alt_signal, u_out))
    return false;
  if (!cblock->set_factor(u_out, c_factor))
    return false;
  if (!iblock->connect(u_out, i_out))
    return false;
  return true;
}

void lucidac::LUCIDAC::reset(bool keep_calibration) {
  for (auto block : get_blocks()) {
    if (block)
      block->reset(keep_calibration);
  }
}

entities::Entity *lucidac::LUCIDAC::get_child_entity(const std::string &child_id) {
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
  return nullptr;
}

bool lucidac::LUCIDAC::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  // Cluster has no own configuration parameters currently
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

std::vector<entities::Entity *> lucidac::LUCIDAC::get_child_entities() {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  return {m1block, m2block, ublock, cblock, iblock};
}
