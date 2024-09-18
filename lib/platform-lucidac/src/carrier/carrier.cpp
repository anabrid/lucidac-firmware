// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier.h"
#include "daq/daq.h"
#include "net/settings.h"
#include "utils/is_number.h"

FLASHMEM entities::EntityClass carrier::Carrier::get_entity_class() const {
  return entities::EntityClass::CARRIER;
}

// Carrier lacks EEPROM
FLASHMEM std::array<uint8_t, 8> carrier::Carrier::get_entity_eui() const { return {0}; }

FLASHMEM carrier::Carrier::Carrier(std::vector<Cluster> clusters, carrier::Carrier_HAL *hardware)
    : Entity(""), hardware(hardware), clusters(std::move(clusters)) {}

FLASHMEM bool carrier::Carrier::init() {
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

FLASHMEM std::vector<entities::Entity *> carrier::Carrier::get_child_entities() {
  std::vector<entities::Entity *> children;
  for (auto &cluster : clusters) {
    children.push_back(&cluster);
  }
  if (ctrl_block)
    children.push_back(ctrl_block);
  return children;
}

FLASHMEM entities::Entity *carrier::Carrier::get_child_entity(const std::string &child_id) {
  if (utils::is_number(child_id.begin(), child_id.end())) {
    auto cluster_idx = std::stoul(child_id);
    if (cluster_idx < 0 or clusters.size() < cluster_idx)
      return nullptr;
    return &clusters[cluster_idx];
  }
  if (child_id == "CTRL")
    return ctrl_block;
  return nullptr;
}

FLASHMEM utils::status carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
  // TODO: Have an option to fail on unexpected configuration
  return utils::status::success();
}

FLASHMEM utils::status carrier::Carrier::write_to_hardware() {
  utils::status error;
  size_t cluster_index = 0;
  for (auto &cluster : clusters) {
    if (!cluster.write_to_hardware()) {
      LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
      error.code += cluster_index; // maxes to 3 = 0b11
      error.msg += "Cluster write failed.";
    }
    cluster_index++;
  }
  if (ctrl_block && !ctrl_block->write_to_hardware()) {
    error.code |= 0x4;
    error.msg += "CTRL Block write failed.";
  }
  if (!hardware->write_adc_bus_mux(adc_channels)) {
    error.code |= 0x8;
    error.msg += "ADC Bus write failed.";
  }
  return error;
}

FLASHMEM bool carrier::Carrier::calibrate_offset() {
  for (auto &cluster : clusters)
    if (!cluster.calibrate_offsets())
      return false;
  return true;
}

FLASHMEM bool carrier::Carrier::calibrate_routes_in_cluster(Cluster &cluster, daq::BaseDAQ *daq_) {
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

FLASHMEM bool carrier::Carrier::calibrate_routes(daq::BaseDAQ *daq_) {
  for (auto &cluster : clusters) {
    if (!calibrate_routes_in_cluster(cluster, daq_))
      return false;
  }
  return true;
}

FLASHMEM bool carrier::Carrier::calibrate_mblock(Cluster &cluster, blocks::MBlock &mblock,
                                                 daq::BaseDAQ *daq_) {
  // CARE: This function does not preserve the currently configured routes
  LOG(ANABRID_DEBUG_CALIBRATION, __PRETTY_FUNCTION__);

  bool success = true;

  // The calibration for each M-Block is prepared by connecting calibrated signals
  // to all eight of its input. Afterwards, the control is passed to the specific
  // M-Block calibration routine, which might change the value of those signals.
  // Additionally, all outputs are routed to the ADC. This limits the calibration to
  // one M-Block (and one cluster) at a time, which is not a problem though.

  LOG(ANABRID_DEBUG_CALIBRATION, "Preparing calibration signal for M-block...");
  for (auto input_idx : blocks::MBlock::SLOT_INPUT_IDX_RANGE()) {
    auto lane_idx = mblock.slot_to_global_io_index(input_idx);
    if (!cluster.add_constant(blocks::UBlock::Transmission_Mode::POS_REF, lane_idx, 1.0f, lane_idx))
      return false;
  }

  LOG(ANABRID_DEBUG_CALIBRATION, "Connecting outputs to ADC...");
  for (auto output_idx : blocks::MBlock::SLOT_OUTPUT_IDX_RANGE()) {
    auto lane_idx = mblock.slot_to_global_io_index(output_idx);
    if (!set_adc_channel(output_idx, lane_idx))
      return false;
  }

  // Write to hardware
  // TODO: could be improved to not write every cluster
  if (!write_to_hardware())
    return false;

  // Run calibration on the reference signals
  LOG(ANABRID_DEBUG_CALIBRATION, "Calibrating calibration signals...");
  if (!calibrate_routes_in_cluster(cluster, daq_))
    return false;

  // Pass to calibration function
  LOG(ANABRID_DEBUG_CALIBRATION, "Passing control to M-block...");
  success &= mblock.calibrate(daq_, &cluster);

  // Clean up
  // reset(true);
  cluster.reset(true);
  ctrl_block->reset(true);
  reset_adc_channels();

  // Write final clean-up to hardware
  // TODO: could be improved to not write every cluster
  if (!write_to_hardware())
    return false;

  return success;
}

FLASHMEM bool carrier::Carrier::calibrate_m_blocks(daq::BaseDAQ *daq_) {
  bool success = true;
  for (auto &cluster : clusters)
    for (auto mblock : {cluster.m0block, cluster.m1block}) {
      if (mblock)
        if (!calibrate_mblock(cluster, *mblock, daq_))
          success = false;
    }
  return success;
}

FLASHMEM void carrier::Carrier::reset(entities::ResetAction action) {
  for (auto &cluster : clusters) {
    cluster.reset(action);
  }
  if (ctrl_block)
    ctrl_block->reset(action);
  reset_adc_channels();
}

const std::array<int8_t, 8> &carrier::Carrier::get_adc_channels() const { return adc_channels; }

FLASHMEM bool carrier::Carrier::set_adc_channels(const std::array<int8_t, 8> &channels) {
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

FLASHMEM bool carrier::Carrier::set_adc_channel(uint8_t idx, int8_t adc_channel) {
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

FLASHMEM void carrier::Carrier::reset_adc_channels() {
  std::fill(adc_channels.begin(), adc_channels.end(), ADC_CHANNEL_DISABLED);
}

FLASHMEM utils::status carrier::Carrier::user_set_extended_config(JsonObjectConst msg_in,
                                                                  JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif

  bool default_reset_before = true, default_sh_kludge = true, default_mul_calib_kludge = true,
       default_calibrate_mblock = false, default_calibrate_offset = false, default_calibrate_routes = false;

  if (msg_in["reset_before"] | default_reset_before) {
    reset(entities::ResetAction::CIRCUIT_RESET | entities::ResetAction::OVERLOAD_RESET |
          entities::ResetAction::CALIBRATION_RESET);
    write_to_hardware();
  }

  if (msg_in["sh_kludge"] | default_sh_kludge) {
    for (auto &cluster : clusters) {
      if (cluster.shblock) {
        cluster.shblock->set_state(blocks::SHBlock::State::TRACK);
        /*cluster.shblock->*/ write_to_hardware();
        delayMicroseconds(1000);
        cluster.shblock->set_state(blocks::SHBlock::State::INJECT);
        /*cluster.shblock->*/ write_to_hardware();
      }
    }
  }

  if (msg_in["mul_calib_kludge"] | default_mul_calib_kludge) {
    blocks::MMulBlock *mulblock;
    for (auto &cluster : clusters) {
      if (cluster.m0block->is_entity_type(blocks::MMulBlock::TYPE))
        mulblock = (blocks::MMulBlock *)cluster.m0block;
      if (cluster.m1block->is_entity_type(blocks::MMulBlock::TYPE))
        mulblock = (blocks::MMulBlock *)cluster.m1block;
    }
    auto res = mulblock->read_calibration_from_eeprom();
    if (!res)
      return res.attach("(mul_calib_kludge)");
    res = mulblock->write_calibration_to_hardware();
    if (!res)
      return res.attach("(mul_calib_kludge)");
  }

  daq::OneshotDAQ daq;
  if ((msg_in["calibrate_mblock"] | default_calibrate_mblock) && !calibrate_m_blocks(&daq))
    return utils::status(10, "Calibrate MBlocks failed");

  auto res = user_set_config(msg_in, msg_out);

  if (!res)
    return res;

  if ((msg_in["calibrate_offset"] | default_calibrate_offset) && !calibrate_offset())
    return utils::status(10, "Calibrate-offset failed");

  if ((msg_in["calibrate_routes"] | default_calibrate_routes) && !calibrate_routes(&daq))
    return utils::status(10, "Calibrate-Routes failed");

  return utils::status::success();
}
