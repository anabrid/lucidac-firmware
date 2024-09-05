// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier.h"
#include "daq/daq.h"
#include "net/settings.h"
#include "utils/is_number.h"

entities::EntityClass carrier::Carrier::get_entity_class() const { return entities::EntityClass::CARRIER; }

// Carrier lacks EEPROM
std::array<uint8_t, 8> carrier::Carrier::get_entity_eui() const { return {0}; }

carrier::Carrier::Carrier(std::vector<Cluster> clusters, carrier::Carrier_HAL *hardware)
    : Entity(""), hardware(hardware), clusters(std::move(clusters)) {}

bool carrier::Carrier::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);

#ifdef ARDUINO
  entity_id = net::StartupConfig::get().mac;
#else
  entity_id = "Placeholder ID";
#endif

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
  if (utils::is_number(child_id.begin(), child_id.end())) {
    auto cluster_idx = std::stoul(child_id);
    if (cluster_idx < 0 or clusters.size() < cluster_idx)
      return nullptr;
    return &clusters[cluster_idx];
  }
  return nullptr;
}

utils::status carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
  // TODO: Have an option to fail on unexpected configuration
  return utils::status::success();
}

int carrier::Carrier::write_to_hardware() {
  for (auto &cluster : clusters) {
    if (!cluster.write_to_hardware()) {
      LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
      return -1;
    }
  }
  if (ctrl_block && !ctrl_block->write_to_hardware())
    return -2;
  if (!hardware->write_adc_bus_mux(adc_channels))
    return -3;
  return true;
}

bool carrier::Carrier::calibrate_offset() {
  for (auto &cluster : clusters)
    if (!cluster.calibrate_offsets())
      return false;
  return true;
}

bool carrier::Carrier::calibrate_routes_in_cluster(Cluster &cluster, daq::BaseDAQ *daq_) {
  ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());
  if (!ctrl_block->write_to_hardware())
    return false;
  if (!cluster.calibrate_routes(daq_))
    return false;
  ctrl_block->reset_adc_bus();
  if (!ctrl_block->write_to_hardware())
    return false;
  return true;
}

bool carrier::Carrier::calibrate_routes(daq::BaseDAQ *daq_) {
  for (auto &cluster : clusters) {
    if (!calibrate_routes_in_cluster(cluster, daq_))
      return false;
  }
  return true;
}

utils::status carrier::Carrier::calibrate_mblock(Cluster &cluster, blocks::MBlock &mblock, daq::BaseDAQ *daq_, bool calibrate_cluster) {
  // CARE: This function does not preserve the currently configured routes
  LOG(ANABRID_DEBUG_CALIBRATION, __PRETTY_FUNCTION__);

  // The calibration for each M-Block is prepared by connecting calibrated signals
  // to all eight of its input. Afterwards, the control is passed to the specific
  // M-Block calibration routine, which might change the value of those signals.
  // Additionally, all outputs are routed to the ADC. This limits the calibration to
  // one M-Block (and one cluster) at a time, which is not a problem though.

  LOG(ANABRID_DEBUG_CALIBRATION, "Preparing calibration signal for M-block...");
  for (auto input_idx : blocks::MBlock::SLOT_INPUT_IDX_RANGE()) {
    auto lane_idx = mblock.slot_to_global_io_index(input_idx);
    if (!cluster.add_constant(blocks::UBlock::Transmission_Mode::POS_REF, lane_idx, 1.0f, lane_idx))
      return utils::status(1);
  }

  LOG(ANABRID_DEBUG_CALIBRATION, "Connecting outputs to ADC...");
  reset_adc_channels();
  for (auto output_idx : blocks::MBlock::SLOT_OUTPUT_IDX_RANGE()) {
    auto lane_idx = mblock.slot_to_global_io_index(output_idx);
    if (!set_adc_channel(output_idx, lane_idx))
      return utils::status(2);
  }

  // Write to hardware
  // TODO: could be improved to not write every cluster
  if (!write_to_hardware())
    return utils::status(3);

  // Run calibration on the reference signals
  if(calibrate_cluster) {
    LOG(ANABRID_DEBUG_CALIBRATION, "Calibrating calibration signals...");
    if (!calibrate_routes_in_cluster(cluster, daq_))
      return utils::status(4);
  } else {
    LOG(ANABRID_DEBUG_CALIBRATION, "Skipping error-prone Cluster calibration...");
  }

  // Pass to calibration function
  LOG(ANABRID_DEBUG_CALIBRATION, "Passing control to M-block...");
  auto res = mblock.calibrate(daq_, *this, cluster);
  if(!res) {
    return res;
  }

  // Clean up
  cluster.reset(true);
  reset_adc_channels();

  // Write final clean-up to hardware
  // TODO: could be improved to not write every cluster
  if (!write_to_hardware())
    return utils::status(5);

  return utils::status(0);
}

void carrier::Carrier::reset(bool keep_calibration) {
  for (auto &cluster : clusters) {
    cluster.reset(keep_calibration);
  }
  if (ctrl_block)
    ctrl_block->reset(keep_calibration);
  reset_adc_channels();
}

const std::array<int8_t, 8> &carrier::Carrier::get_adc_channels() const { return adc_channels; }

bool carrier::Carrier::set_adc_channels(const std::array<int8_t, 8> &channels) {
  // Check that all inputs are < 15
  for (auto channel : channels) {
    if (channel > 15)
      return false;
  }
  // Check that all inputs are different
  // This is actually not necessary, in principle one could split one signal to multiple ADC.
  // But for now we prohibit this, as it's more likely an error than an actual use-case.
  for (auto &channel : channels) {
    if (channel < 0)
      continue;
    for (auto &other_channel : channels) {
      if (std::addressof(channel) == std::addressof(other_channel))
        continue;
      if (channel == other_channel)
        return false;
    }
  }
  adc_channels = channels;
  return true;
}

bool carrier::Carrier::set_adc_channel(uint8_t idx, int8_t adc_channel) {
  if (idx >= adc_channels.size())
    return false;
  if (adc_channel < 0)
    adc_channel = ADC_CHANNEL_DISABLED;

  // Check if channel is already routed to an ADC
  // This is not really necessary, but probably a good idea (see set_adc_channels)
  if (adc_channel != ADC_CHANNEL_DISABLED)
    for (auto other_idx = 0u; other_idx < adc_channels.size(); other_idx++)
      if (idx != other_idx and adc_channels[other_idx] == adc_channel)
        return false;

  adc_channels[idx] = adc_channel;
  return true;
}

void carrier::Carrier::reset_adc_channels() {
  std::fill(adc_channels.begin(), adc_channels.end(), ADC_CHANNEL_DISABLED);
}

constexpr int error(int i) { return i; } // just some syntactic sugar

constexpr int success = 0;

int carrier::Carrier::set_config(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto self_entity_id = get_entity_id();
  if (!msg_in.containsKey("entity") or !msg_in.containsKey("config")) {
    msg_out["error"] = "Malformed message.";
    return error(1);
  }

  // Convert JSON array of possible anything to string array
  auto path_json = msg_in["entity"].as<JsonArrayConst>();
  auto path_depth = path_json.size();
  std::string path[path_depth];
  copyArray(path_json, path, path_depth);

  // Sanity check path, which must at least be addressed to us
  if (!path_depth) {
    msg_out["error"] = "Invalid entity path.";
    return error(2);
  }
  if (path[0] != self_entity_id) {
    msg_out["error"] = "Message intended for another carrier.";
    return error(3);
  }

  daq::OneshotDAQ daq;
  if(msg_in.containsKey("calibrate_mblock")) {
    utils::status ret(0);
    if(clusters[0].m0block && clusters[0].m0block->is_entity_type(blocks::MBlock::TYPES::M_MUL4_BLOCK))
      ret = calibrate_mblock(clusters[0], *clusters[0].m0block, &daq);
    if (clusters[0].m1block && clusters[0].m1block->is_entity_type(blocks::MBlock::TYPES::M_MUL4_BLOCK))
      ret = calibrate_mblock(clusters[0], *clusters[0].m1block, &daq);
    if(!ret) {
      msg_out["error"] = std::string("Calibrate MBlock failed: ") + std::string(ret.msg);
      return error(100 + ret.code);
    }
  }

  // Path may be to one of our sub-entities
  auto resolved_entity = resolve_child_entity(path + 1, path_depth - 1);
  if (!resolved_entity) {
    msg_out["error"] = utils::small_sprintf("Carrier: No entity named '%s'", path[1].c_str());
    return error(4);
  }

  utils::status res = resolved_entity->config_from_json(msg_in["config"]);
  if (!res && msg_out["error"].isNull()) {
    msg_out["error"] = utils::small_sprintf("Could not apply configuration to entity '%s', error: %s",
                                            resolved_entity->get_entity_id().c_str(), res.msg.c_str());
    return error(10'000 + res.code);
  }

  // Actually write to hardware
  int hw_res = write_to_hardware();
  switch (hw_res) {
  case -1:
    msg_out["error"] = "Cluster write failed";
    return error(6);
  case -2:
    msg_out["error"] = "CTRL Block write failed";
    return error(7);
  case -3:
    msg_out["error"] = "ADC Bus write failed";
    return error(8);

  // This weird case captures the illegal code fact that write_to_hardware() per-interface
  // has a boolean return value but for a few classes an integer return value. This needs
  // to be fixed.
  case false:
    msg_out["error"] = "Generic write to hardware error";
    return error(9);

  case true: /* boolean success */
  default:
    return success;
  }

  if (msg_in.containsKey("calibrate_offset") && !calibrate_offset()) {
    msg_out["error"] = "Calibrate-offset failed";
    return error(10);
  }

  if (msg_in.containsKey("calibrate_routes") && !calibrate_routes(&daq)) {
    msg_out["error"] = "Calibrate-Routes failed";
    return error(10);
  }
}

int carrier::Carrier::get_config(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto recursive = true;
  if (msg_in.containsKey("recursive"))
    recursive = msg_in["recursive"].as<bool>();

  // Message may contain path to sub-entity
  entities::Entity *entity = nullptr;
  if (!msg_in.containsKey("entity") or msg_in["entity"].isNull()) {
    entity = this;
  } else if (msg_in["entity"].is<JsonArrayConst>()) {
    auto path = msg_in["entity"].as<JsonArrayConst>();
    if (!path.size()) {
      entity = this;
    } else if (path[0].as<std::string>() != get_entity_id()) {
      msg_out["error"] = "Entity lives on another carrier.";
      return error(1);
    } else {
      auto path_begin = path.begin();
      ++path_begin;
      entity = resolve_child_entity(path_begin, path.end());
      if (!entity) {
        msg_out["error"] = "Invalid entity path.";
        return error(2);
      }
    }
  } else {
    msg_out["error"] = "Invalid entity path.";
    return error(3);
  }

  // Save entity path back into response
  msg_out["entity"] = msg_in["entity"];
  // Save config into response
  auto cfg = msg_out.createNestedObject("config");
  entity->config_to_json(cfg, recursive);
  return success;
}

int carrier::Carrier::get_entities(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto entities_obj = msg_out.createNestedObject("entities");
  auto carrier_obj = entities_obj[get_entity_id()] = get_entity_classifier();
  for (const auto &cluster : clusters) {
    auto cluster_obj = carrier_obj["/" + cluster.get_entity_id()] = cluster.get_entity_classifier();
    for (auto *block : cluster.get_blocks()) {
      if (block) {
        auto block_obj = cluster_obj["/" + block->get_entity_id()] = block->get_entity_classifier();
        block_obj["eui"] = utils::toString(block->get_entity_eui());
      }
    }
  }
  return success;
}

int carrier::Carrier::reset(JsonObjectConst msg_in, JsonObject &msg_out) {
  for (auto &cluster : clusters) {
    cluster.reset(msg_in["keep_calibration"] | true);
  }
  if (msg_in["sync"] | true) {
    write_to_hardware();
  }
  return success;
}
