// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/mblock.h"
#include "utils/logging.h"

#include "carrier/cluster.h"

blocks::MMulBlock::MMulBlock(bus::addr_t block_address, MMulBlockHAL *hardware)
    : MBlock(block_address), hardware(hardware) {}

bool blocks::MMulBlock::init() {
  // TODO: Remove once hardware pointer is part of FunctionBlock base class
  return FunctionBlock::init() and hardware->init();
}

void blocks::MMulBlock::reset(bool keep_calibration) {
  if (!keep_calibration) {
    for (auto idx = 0u; idx < NUM_MULTIPLIERS; idx++) {
      calibration[idx].offset_x = 0;
      calibration[idx].offset_y = 0;
      calibration[idx].offset_z = 0;
    }
  }
}

bool blocks::MMulBlock::write_to_hardware() { return true; }

utils::status blocks::MMulBlock::calibrate(daq::BaseDAQ *daq_, carrier::Carrier &carrier_,
                                           platform::Cluster &cluster, bool calibrate_cluster) {
  LOG(ANABRID_DEBUG_CALIBRATION, __PRETTY_FUNCTION__);
  if (!MBlock::calibrate(daq_, carrier_, cluster))
    return utils::status("Mblock::calibrate failed");

  // TODO: Potentially reset current calibration

  // Our first simple calibration process has been developed empirically and goes
  // 1. Set all inputs to zero
  //    - The input offsets are rather small, so inputs are 0+-epsilon
  //    - The output is then roughly 0 + epsilon^2 - offset_z ~= -offset_z
  // 2. Measure output and use it as a first estimate of offset_z
  // 3. Set inputs to 0 and 1 and change the first one's offset to get an output close to zero
  // 4. Repeat (3), switching inputs

  // TODO: Somehow this must be interleaved with the cluster calibration,
  //       or rather we might want to call this with a certain default routing set up?

  // Connect zeros to all our inputs
  LOG(ANABRID_DEBUG_CALIBRATION, "Calibrating output offsets...");
  for (auto idx : SLOT_INPUT_IDX_RANGE())
    if (!cluster.cblock->set_factor(slot_to_global_io_index(idx), 0.0f))
      return false;
  if (!cluster.cblock->write_to_hardware())
    return false;
  // When changing a factor, we always have to calibrate offset
  if (calibrate_cluster)
    cluster.calibrate_offsets();
  else
    LOG(ANABRID_DEBUG_CALIBRATION, "Skipping error-prone cluster calibration...");

  // Measure offset_z and set it
  auto offset_zs = daq_->sample();
  for (auto idx = 0u; idx < NUM_MULTIPLIERS; idx++) {
    if (!hardware->write_calibration_output_offset(idx, -offset_zs[idx]))
      return false;
    calibration[idx].offset_z = -offset_zs[idx];
  }

  // Set some inputs to one
  LOG(ANABRID_DEBUG_CALIBRATION, "Calibrating input x offsets...");
  for (auto idx = 0u; idx < NUM_MULTIPLIERS; idx++)
    if (!cluster.cblock->set_factor(slot_to_global_io_index(idx * 2), 1.0f))
      return false;
  if (!cluster.cblock->write_to_hardware())
    return false;
  // When changing a factor, we always have to calibrate offset
  if (calibrate_cluster)
    cluster.calibrate_offsets();
  else
    LOG(ANABRID_DEBUG_CALIBRATION, "Skipping error-prone cluster calibration...");
  delay(100);

  // Start with a negative input offset and increase until we hit/cross zero
  for (auto mul_idx = 0u; mul_idx < NUM_MULTIPLIERS; mul_idx++) {
    if (!hardware->write_calibration_input_offsets(mul_idx, -0.1f, 0.0f))
      return utils::status("mblock write_calibration_input_offsets failed for (mul_idx=%d, -0.1f, 0.0f)",
                           mul_idx);
    calibration[mul_idx].offset_x = -0.1f;
    for (;;) {
      auto measured = daq_->sample(mul_idx);
      if (measured >= 0.0) {
        if (calibration[mul_idx].offset_x >= 0.1) {
          LOGMEV("MBlock calibration mul_idx=%d at limit for offset_x=%f measured=%f", mul_idx,
                 calibration[mul_idx].offset_x, measured);
          break; // either raise an error here or continue "silently"
        }
        if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x, 0.0f))
          return utils::status("mblock write_to_calibration for offset_x=%f failed.",
                               calibration[mul_idx].offset_x);
        calibration[mul_idx].offset_x += 0.01f;
        delay(7);
      } else {
        LOGMEV("MBlock calibration mul_idx=%d completed for offset_x=%f measured=%f", mul_idx,
               calibration[mul_idx].offset_x, measured);
        break;
      }
    }
  }

  // Set other inputs to one
  LOG(ANABRID_DEBUG_CALIBRATION, "Calibrating input y offsets...");
  for (auto idx = 0u; idx < NUM_MULTIPLIERS; idx++) {
    if (!cluster.cblock->set_factor(slot_to_global_io_index(idx * 2), 0.0f))
      return false;
    if (!cluster.cblock->set_factor(slot_to_global_io_index(idx * 2 + 1), 1.0f))
      return false;
  }
  if (!cluster.cblock->write_to_hardware())
    return false;
  // When changing a factor, we always have to calibrate offset
  if (calibrate_cluster)
    cluster.calibrate_offsets();
  else
    LOG(ANABRID_DEBUG_CALIBRATION, "Skipping error-prone cluster calibration...");
  delay(100);

  // Start with a negative input offset and increase until we hit/cross zero
  for (auto mul_idx = 0u; mul_idx < NUM_MULTIPLIERS; mul_idx++) {
    if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x, -0.1f))
      return utils::status("mblock write_calibration_input_offsets failed for (mul_idx=%d, %f, -0.1f)",
                           mul_idx, calibration[mul_idx].offset_x);
    calibration[mul_idx].offset_y = -0.1f;
    for (;;) {
      auto measured = daq_->sample(mul_idx);
      if (measured >= 0.0) {
        if (calibration[mul_idx].offset_y >= 0.1) {
          LOGMEV("MBlock calibration mul_idx=%d at limit for offset_y=%f measured=%f with offset_x=%f",
                 mul_idx, calibration[mul_idx].offset_y, measured, calibration[mul_idx].offset_x);
        }
        if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x,
                                                       calibration[mul_idx].offset_y))
          return utils::status("mblock write_to_calibration for offset_y=%f with offset_x=%f failed",
                               calibration[mul_idx].offset_y, calibration[mul_idx].offset_x);
        calibration[mul_idx].offset_y += 0.01f;
        delay(7);
      } else {
        LOGMEV("MBlock calibration mul_idx=%d completed for offset_y=%f measured=%f with offset_x=%f", mul_idx,
               calibration[mul_idx].offset_y, measured, calibration[mul_idx].offset_x);
        break;
      }
    }
  }

  return true;
}

const std::array<blocks::MultiplierCalibration, blocks::MMulBlock::NUM_MULTIPLIERS> &
blocks::MMulBlock::get_calibration() const {
  return calibration;
}

blocks::MultiplierCalibration blocks::MMulBlock::get_calibration(uint8_t mul_idx) const {
  if (mul_idx >= NUM_MULTIPLIERS)
    return {};
  return calibration[mul_idx];
}

utils::status blocks::MMulBlock::config_self_from_json(JsonObjectConst cfg) {
  // MMulBlock does not expect any configuration currently.
  // But due to automation, some may still be sent.
  // Thus we accept any configuration containing only empty values or similar.
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "calibration") {
      auto res = _config_elements_from_json(cfgItr->value());
      if (!res)
        return res;
    } else {
      return utils::status("MMulBlock: Unknown configuration key");
    }
  }
  return utils::status::success();
}

utils::status blocks::MMulBlock::_config_elements_from_json(const JsonVariantConst &cfg) {
  // return utils::status("MMulBlock currently does not accept configuration");

  // Note: The following is a hack.
  //       We generally don't want clients to take over calibration.

  auto map = cfg.as<JsonObjectConst>();
  if (!map.containsKey("offset_x") || map["offset_x"].size() != MMulBlock::NUM_MULTIPLIERS)
    return utils::status("Missing offset_x (need 4 entries)");
  if (!map.containsKey("offset_y") || map["offset_y"].size() != MMulBlock::NUM_MULTIPLIERS)
    return utils::status("Missing offset_y (need 4 entries)");
  if (!map.containsKey("offset_z") || map["offset_z"].size() != MMulBlock::NUM_MULTIPLIERS)
    return utils::status("Missing offset_z (need 4 entries)");
  for (int i = 0; i < MMulBlock::NUM_MULTIPLIERS; i++) {
    calibration[i].offset_x = map["offset_x"][i];
    calibration[i].offset_y = map["offset_y"][i];
    calibration[i].offset_z = map["offset_z"][i];

    if (!hardware->write_calibration_input_offsets(i, calibration[i].offset_x, calibration[i].offset_y))
      return utils::status(
          "MMulBlock::calibration from json for multiplier %d values offset_x, offset_y not accepted", i);
    if (!hardware->write_calibration_output_offset(i, calibration[i].offset_z))
      return utils::status("MMulBlock::calibration from json for multiplier %d values offset_z not accepted",
                           i);
  }

  return 0;
}

blocks::MMulBlock *blocks::MMulBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                             const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != static_cast<uint8_t>(TYPE))
    return nullptr;

  // Currently, there are no different variants
  if (classifier.variant != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  if (classifier.version < entities::Version(1))
    return nullptr;
  if (classifier.version < entities::Version(1, 1))
    return new MMulBlock(block_address, new MMulBlockHAL_V_1_0_X(block_address));
  return nullptr;
}

// Hardware abstraction layer

bool blocks::MMulBlockHAL::init() {
  if (!FunctionBlockHAL::init())
    return false;
  return reset_calibration_input_offsets() and reset_calibration_output_offsets();
}

bool blocks::MMulBlockHAL_V_1_0_X::init() {
  return f_calibration_dac_0.init() and f_calibration_dac_0.set_external_reference() and
         f_calibration_dac_0.set_double_gain() and f_calibration_dac_1.init() and
         f_calibration_dac_1.set_external_reference() and f_calibration_dac_1.set_double_gain() and
         MMulBlockHAL::init();
}

blocks::MMulBlockHAL_V_1_0_X::MMulBlockHAL_V_1_0_X(bus::addr_t block_address)
    : f_calibration_dac_0(bus::address_from_tuple(block_address, 4)),
      f_calibration_dac_1(bus::address_from_tuple(block_address, 5)) {}

bool blocks::MMulBlockHAL_V_1_0_X::write_calibration_input_offsets(uint8_t idx, float offset_x,
                                                                   float offset_y) {
  // Supported offset range is [-0.1V, +0.1V], which corresponds to [-0.1, 0.1] machine units,
  // since the multiplier's working range is [-1V, +1V].
  if (fabs(offset_x) > 0.1f or fabs(offset_y) > 0.1f)
    return false;
  // Note: Mapping of index to channel is surprising
  // Note: Mapping of offset values to DAC values is surprising, since
  //       the current DAC implementation assumes a 2.5V reference, but we have a 2V reference here.
  //       With gain=2, passing 2.5 gives 4V output, which is scaled and level shifted to -0.1V.
  //       Passing 0 gives 0V output, which is scaled and shifted to +0.1V
  //       It turns out the I/U-converters invert BL_IN, so we want to invert here as well.
  return f_calibration_dac_0.set_channel(idx * 2 + 1, (offset_x - 0.1f) * -12.5f) and
         f_calibration_dac_0.set_channel(idx * 2, (offset_y - 0.1f) * -12.5f);
}

bool blocks::MMulBlockHAL_V_1_0_X::reset_calibration_input_offsets() {
  for (auto idx = 0u; idx < MMulBlock::NUM_MULTIPLIERS; idx++)
    if (!write_calibration_input_offsets(idx, 0, 0))
      return false;
  return true;
}

bool blocks::MMulBlockHAL_V_1_0_X::reset_calibration_output_offsets() {
  for (auto idx = 0u; idx < MMulBlock::NUM_MULTIPLIERS; idx++)
    if (!write_calibration_output_offset(idx, 0))
      return false;
  return true;
}

bool blocks::MMulBlockHAL_V_1_0_X::write_calibration_output_offset(uint8_t idx, float offset_z) {
  // See write_calibration_input_offsets for some explanations.
  if (fabs(offset_z) > 0.1f)
    return false;
  return f_calibration_dac_1.set_channel(idx, (offset_z - 0.1f) * -12.5f);
}
