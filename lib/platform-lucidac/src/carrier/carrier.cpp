// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier.h"

entities::EntityClass carrier::Carrier::get_entity_class() const { return entities::EntityClass::CARRIER; }

carrier::Carrier::Carrier(std::vector<Cluster> clusters) : Entity(""), clusters(std::move(clusters)) {}

bool carrier::Carrier::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  entity_id = get_system_mac();
  LOG(ANABRID_DEBUG_INIT, entity_id.c_str());
  if (entity_id.empty())
    return false;

  // Detect CTRL-block
  ctrl_block = entities::detect<blocks::CTRLBlock>(bus::address_from_tuple(1, 0));
  if (!ctrl_block)
    return false;

  for (auto &cluster : clusters) {
    if (!cluster.init())
      return false;
  }
  return true;
}

std::vector<entities::Entity *> carrier::Carrier::get_child_entities() {
  std::vector<entities::Entity *> children;
  for (auto &cluster : clusters) {
    children.push_back(&cluster);
  }
  return children;
}

entities::Entity *carrier::Carrier::get_child_entity(const std::string &child_id) {
  if (is_number(child_id.begin(), child_id.end())) {
    auto cluster_idx = std::stoul(child_id);
    if (cluster_idx < 0 or clusters.size() < cluster_idx)
      return nullptr;
    return &clusters[cluster_idx];
  }
  return nullptr;
}

bool carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

bool carrier::Carrier::write_to_hardware() {
  for (auto &cluster : clusters) {
    if (!cluster.write_to_hardware()) {
      LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
      return false;
    }
  }
  return true;
}

bool carrier::Carrier::calibrate(daq::BaseDAQ *daq_) {
  for (auto &cluster : clusters) {
    ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());
    if (!ctrl_block->write_to_hardware())
      return false;
    if (!cluster.calibrate(daq_))
      return false;
  }
  ctrl_block->reset_adc_bus();
  if (!ctrl_block->write_to_hardware())
    return false;
  return true;
}

void carrier::Carrier::reset(bool keep_calibration) {
  for (auto &cluster : clusters) {
    cluster.reset(keep_calibration);
  }
  if (ctrl_block)
    ctrl_block->reset(keep_calibration);
}
