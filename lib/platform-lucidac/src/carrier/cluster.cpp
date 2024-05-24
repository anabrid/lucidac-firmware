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

std::array<blocks::FunctionBlock *, 5> lucidac::LUCIDAC::get_blocks() const {
  return {m1block, m2block, ublock, cblock, iblock};
}

bool lucidac::LUCIDAC::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  bus::init();

  // Dynamically detect installed blocks
  // Check if a block is already set, which may happen with a special constructor in the future
  LOG(ANABRID_DEBUG_INIT, "Detecting installed blocks...");
  if (!m1block) {
    m1block = blocks::detect<blocks::MBlock>(bus::idx_to_addr(cluster_idx, bus::M1_BLOCK_IDX, 0));
    if (!m1block)
      LOG(ANABRID_DEBUG_INIT, "Warning: M0-block is missing or unknown.");
  }
  if (!m2block) {
    m2block = blocks::detect<blocks::MBlock>(bus::idx_to_addr(cluster_idx, bus::M2_BLOCK_IDX, 0));
    if (!m2block)
      LOG(ANABRID_DEBUG_INIT, "Warning: M1-block is missing or unknown.");
  }
  if (!m1block and !m2block) {
    LOG(ANABRID_DEBUG_INIT, "Error: Both M0 and M1-blocks are missing or unknown.");
    return false;
  }

  if (!ublock) {
    ublock = blocks::detect<blocks::UBlock>(bus::idx_to_addr(cluster_idx, bus::U_BLOCK_IDX, 0));
    if (!ublock) {
      LOG(ANABRID_DEBUG_INIT, "Error: U-block is missing or unknown.");
      return false;
    }
  }
  if (!cblock) {
    cblock = blocks::detect<blocks::CBlock>(bus::idx_to_addr(cluster_idx, bus::C_BLOCK_IDX, 0));
    if (!cblock) {
      LOG(ANABRID_DEBUG_INIT, "Error: C-block is missing or unknown.");
      return false;
    }
  }
  if (!iblock) {
    iblock = blocks::detect<blocks::IBlock>(bus::idx_to_addr(cluster_idx, bus::I_BLOCK_IDX, 0));
    if (!iblock) {
      LOG(ANABRID_DEBUG_INIT, "Error: I-block is missing or unknown.");
      return false;
    }
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

lucidac::LUCIDAC::LUCIDAC(uint8_t cluster_idx)
    : entities::Entity(std::to_string(cluster_idx)), cluster_idx(cluster_idx) {}

bool lucidac::LUCIDAC::calibrate(daq::BaseDAQ *daq) {
  // Return to a non-connected state, but keep calibrated offsets
  reset(true);
  write_to_hardware();
  return true;
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
