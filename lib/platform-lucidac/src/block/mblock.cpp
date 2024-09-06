// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/mblock.h"
#include "utils/logging.h"

#include "carrier/cluster.h"

#include "io/io.h" // Just for testing TODO REMOVE
#include "utils/logging.h"

blocks::MBlock::MBlock(bus::addr_t block_address)
    : blocks::FunctionBlock{std::string("M") + std::string(
                                                   // Addresses 12, 20, 28 are M0
                                                   // Addresses 13, 21, 29 are M1
                                                   block_address % 8 == 4 ? "0" : "1"),
                            block_address},
      slot(block_address % 8 == 4 ? SLOT::M0 : SLOT::M1) {}

uint8_t blocks::MBlock::slot_to_global_io_index(uint8_t local) const {
  switch (slot) {
  case SLOT::M0:
    return local;
  case SLOT::M1:
    return local + 8;
  }
  // This should never be reached
  return local;
}

blocks::MIntBlock::MIntBlock(bus::addr_t block_address, MIntBlockHAL *hardware)
    : blocks::MBlock{block_address}, hardware(hardware), ic_values{}, time_factors{} {
  reset_ic_values();
  reset_time_factors();
}

bool blocks::MIntBlock::init() {
  if (!FunctionBlock::init())
    return false;
  if (!hardware->init())
    return false;
  return true;
}

const std::array<float, 8> &blocks::MIntBlock::get_ic_values() const { return ic_values; }

float blocks::MIntBlock::get_ic_value(uint8_t idx) const {
  if (idx >= ic_values.size())
    return 0.0f;
  return ic_values[idx];
}

bool blocks::MIntBlock::set_ic_values(const std::array<float, 8> &ic_values_) {
  for (auto idx = 0u; idx < ic_values_.size(); idx++)
    if (!set_ic_value(idx, ic_values_[idx]))
      return false;
  return true;
}

bool blocks::MIntBlock::set_ic_values(float value) {
  for (auto idx = 0u; idx < ic_values.size(); idx++)
    if (!set_ic_value(idx, value))
      return false;
  return true;
}

bool blocks::MIntBlock::set_ic_value(uint8_t idx, float value) {
  if (idx >= ic_values.size())
    return false;
  if (value > 1.0f or value < -1.0f)
    return false;
  ic_values[idx] = value;
  return true;
}

void blocks::MIntBlock::reset_ic_values() { std::fill(ic_values.begin(), ic_values.end(), 0.0f); }

const std::array<unsigned int, 8> &blocks::MIntBlock::get_time_factors() const { return time_factors; }

unsigned int blocks::MIntBlock::get_time_factor(uint8_t idx) const {
  if (idx >= time_factors.size())
    return 0;
  return time_factors[idx];
}

bool blocks::MIntBlock::set_time_factors(const std::array<unsigned int, 8> &time_factors_) {
  for (auto idx = 0u; idx < time_factors_.size(); idx++)
    if (!set_time_factor(idx, time_factors_[idx]))
      return false;
  return true;
}

bool blocks::MIntBlock::set_time_factors(unsigned int k) {
  for (auto idx = 0u; idx < time_factors.size(); idx++)
    if (!set_time_factor(idx, k))
      return false;
  return true;
}

bool blocks::MIntBlock::set_time_factor(uint8_t int_idx, unsigned int k) {
  if (!(k == 100 or k == 10000))
    return false;
  if (int_idx >= NUM_INTEGRATORS)
    return false;
  time_factors[int_idx] = k;
  return true;
}

void blocks::MIntBlock::reset_time_factors() {
  // Copying solves a strange linker issue "relocation against ... in read-only section `.text'"
  auto default_ = DEFAULT_TIME_FACTOR;
  std::fill(std::begin(time_factors), std::end(time_factors), default_);
}

bool blocks::MIntBlock::write_to_hardware() {
  // Write IC values one channel at a time
  for (decltype(ic_values.size()) i = 0; i < ic_values.size(); i++) {
    if (!hardware->write_ic(i, ic_values[i])) {
      LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
      return false;
    }
  }
  // Write time factor switches by converting to bitset
  std::bitset<NUM_INTEGRATORS> time_factor_switches{};
  for (auto idx = 0u; idx < time_factors.size(); idx++)
    if (time_factors[idx] != DEFAULT_TIME_FACTOR)
      time_factor_switches.set(idx);
  if (!hardware->write_time_factor_switches(time_factor_switches))
    return false;
  return true;
}

void blocks::MIntBlock::reset(bool keep_calibration) {
  FunctionBlock::reset(keep_calibration);
  reset_ic_values();
  reset_time_factors();
}

utils::status blocks::MIntBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "elements") {
      auto res = _config_elements_from_json(cfgItr->value());
      if (!res)
        return res;
    } else {
      return utils::status("MIntBlock: Unknown configuration key");
    }
  }
  return utils::status::success();
}

utils::status blocks::MIntBlock::_config_elements_from_json(const JsonVariantConst &cfg) {
  if (cfg.is<JsonArrayConst>()) {
    auto ints_cfg = cfg.as<JsonArrayConst>();
    if (ints_cfg.size() != NUM_INTEGRATORS) {
      return utils::status("MIntBlock: Provided %d elments but NUM_INTEGRATORS=%d.", ints_cfg.size(),
                           NUM_INTEGRATORS);
    }
    for (size_t i = 0; i < ints_cfg.size(); i++) {
      if (!ints_cfg[i].containsKey("ic") or !ints_cfg[i]["ic"].is<float>())
        return utils::status("MIntBlock, element %d: Requiring IC as float", i);
      if (!set_ic_value(i, ints_cfg[i]["ic"]))
        return utils::status("MIntBlock, element %d: Illegal IC %f", i, ints_cfg[i]["ic"]);
      if (ints_cfg[i].containsKey("k")) {
        if (!ints_cfg[i]["k"].is<unsigned int>())
          return utils::status("MIntBlock, element %d: Requiring time factor 'k' as int", i);
        if (!set_time_factor(i, ints_cfg[i]["k"]))
          return utils::status("MIntBlock, element %d: Illegal time factor %d", i, ints_cfg[i]["k"]);
      }
    }
    return utils::status::success();
  } else if (cfg.is<JsonObjectConst>()) {
    for (JsonPairConst keyval : cfg.as<JsonObjectConst>()) {
      // Value is an object, with any key being optional
      if (!keyval.value().is<JsonObjectConst>())
        return utils::status("MIntBlock configuration value needs to be an object");
      // TODO: Check conversion from string to number
      auto int_idx = std::stoul(keyval.key().c_str());
      if (int_idx >= NUM_INTEGRATORS)
        return utils::status("MIntBlock: Integrator index %d >= NUM_INTEGRATORS = %d", int_idx,
                             NUM_INTEGRATORS);
      auto int_cfg = keyval.value().as<JsonObjectConst>();
      if (int_cfg.containsKey("ic")) {
        if (!int_cfg["ic"].is<float>())
          return utils::status("MIntBlock: Integrator %d IC must be float", int_idx);
        if (!set_ic_value(int_idx, int_cfg["ic"]))
          return utils::status("MIntBlock: Integrator %d IC is illegal: %f", int_idx, int_cfg["ic"]);
      }
      if (int_cfg.containsKey("k")) {
        if (!int_cfg["k"].is<unsigned int>())
          return utils::status("MIntBlock: Integrator %d k must be integer", int_idx);
        if (!set_time_factor(int_idx, int_cfg["k"]))
          return utils::status("MIntBlock: Integrator %d time factor k illegal: %d", int_idx, int_cfg["k"]);
      }
    }
    return utils::status::success();
  }
  return utils::status("MIntBlock: Configuration must either be array or object");
}

void blocks::MIntBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  auto ints_cfg = cfg.createNestedArray("elements");
  for (size_t i = 0; i < NUM_INTEGRATORS; i++) {
    auto int_cfg = ints_cfg.createNestedObject();
    int_cfg["ic"] = ic_values[i];
    int_cfg["k"] = time_factors[i];
  }
}

bool blocks::MMulBlock::write_to_hardware() { return true; }

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
  //return utils::status("MMulBlock currently does not accept configuration");

  // Note: The following is a hack.
  //       We generally don't want clients to take over calibration.

  auto map = cfg.as<JsonObjectConst>();
  if(!map.containsKey("offset_x") || map["offset_x"].size() != MMulBlock::NUM_MULTIPLIERS)
    return utils::status("Missing offset_x (need 4 entries)");
  if(!map.containsKey("offset_y") || map["offset_y"].size() != MMulBlock::NUM_MULTIPLIERS)
    return utils::status("Missing offset_y (need 4 entries)");
  if(!map.containsKey("offset_z") || map["offset_z"].size() != MMulBlock::NUM_MULTIPLIERS)
    return utils::status("Missing offset_z (need 4 entries)");
  for(int i=0; i<MMulBlock::NUM_MULTIPLIERS; i++) {
    calibration[i].offset_x = map["offset_x"][i];
    calibration[i].offset_y = map["offset_y"][i];
    calibration[i].offset_z = map["offset_z"][i];

    if(!hardware->write_calibration_input_offsets(i, calibration[i].offset_x, calibration[i].offset_y))
      return utils::status("MMulBlock::calibration from json for multiplier %d values offset_x, offset_y not accepted", i);
    if(!hardware->write_calibration_output_offset(i, calibration[i].offset_z))
      return utils::status("MMulBlock::calibration from json for multiplier %d values offset_z not accepted", i);
  }

  return 0;
}

// ███████ ███    ██ ████████ ██ ████████ ██    ██     ███████  █████   ██████ ████████  ██████  ██████  ██
// ███████ ███████ ██      ████   ██    ██    ██    ██     ██  ██      ██      ██   ██ ██         ██    ██ ██
// ██   ██ ██ ██      ██ █████   ██ ██  ██    ██    ██    ██      ████       █████   ███████ ██         ██ ██
// ██ ██████  ██ █████   ███████ ██      ██  ██ ██    ██    ██    ██       ██        ██      ██   ██ ██ ██ ██
// ██ ██   ██ ██ ██           ██ ███████ ██   ████    ██    ██    ██       ██        ██      ██   ██  ██████ ██
// ██████  ██   ██ ██ ███████ ███████

blocks::MBlock *blocks::MBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != entities::EntityClass::M_BLOCK)
    return nullptr;

  auto type = classifier.type_as<TYPES>();
  switch (type) {
  case TYPES::UNKNOWN:
    // This is already checked by !classifier above
    return nullptr;
  case TYPES::M_MUL4_BLOCK:
    return MMulBlock::from_entity_classifier(classifier, block_address);
  case TYPES::M_INT8_BLOCK:
    return MIntBlock::from_entity_classifier(classifier, block_address);
  }
  // Any unknown value results in a nullptr here.
  // Adding default case to switch suppresses warnings about missing cases.
  return nullptr;
}

blocks::MIntBlock *blocks::MIntBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                             const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != static_cast<uint8_t>(TYPE))
    return nullptr;

  // Currently, there are no different variants
  if (classifier.variant != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  if (classifier.version < entities::Version(1))
    return nullptr;
  if (classifier.version < entities::Version(1, 1))
    return new MIntBlock(block_address, new MIntBlockHAL_V_1_0_X(block_address));
  return nullptr;
}

blocks::MIntBlockHAL_V_1_0_X::MIntBlockHAL_V_1_0_X(bus::addr_t block_address)
    : f_ic_dac(bus::replace_function_idx(block_address, 4)),
      f_time_factor(bus::replace_function_idx(block_address, 5), true),
      f_time_factor_sync(bus::replace_function_idx(block_address, 6)),
      f_time_factor_reset(bus::replace_function_idx(block_address, 7)) {}

bool blocks::MIntBlockHAL_V_1_0_X::init() {
  if (!MIntBlockHAL::init())
    return false;
  return f_ic_dac.init() and f_ic_dac.set_external_reference(true) and f_ic_dac.set_double_gain(true);
}

bool blocks::MIntBlockHAL_V_1_0_X::write_ic(uint8_t idx, float ic) {
  if (idx >= MIntBlock::NUM_INTEGRATORS)
    return false;
  // Note: The DAC60508 implementation converts values assuming a 2.5V reference,
  //       but we use a 2V external reference here (resulting in the 1.25 factor).
  //       The output is also level-shifted, such that IC = 2V - output.
  //       And 2V equals a 1, since the output is halved after the integrators.
  //       Since we enabled gain=2, we don't need to halve/double here.
  //       Resulting in a shift of -1 and the inversion.
  return f_ic_dac.set_channel(idx, (ic + 1.0f) * 1.25f);
}

bool blocks::MIntBlockHAL_V_1_0_X::write_time_factor_switches(std::bitset<8> switches) {
  if (!f_time_factor.transfer8(static_cast<uint8_t>(switches.to_ulong())))
    return false;
  f_time_factor_sync.trigger();
  return true;
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

blocks::MMulBlock::MMulBlock(bus::addr_t block_address, MMulBlockHAL *hardware)
    : MBlock(block_address), hardware(hardware) {}

bool blocks::MMulBlock::init() {
  // TODO: Remove once hardware pointer is part of FunctionBlock base class
  return FunctionBlock::init() and hardware->init();
}

void blocks::MMulBlock::reset(bool keep_calibration) {
  for(auto idx=0u; idx < NUM_MULTIPLIERS; idx++) {
    calibration[idx].offset_x = 0;
    calibration[idx].offset_y = 0;
    calibration[idx].offset_z = 0;
  }
}


utils::status blocks::MMulBlock::calibrate(daq::BaseDAQ *daq_, carrier::Carrier &carrier_, platform::Cluster &cluster, bool calibrate_cluster) {
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
  if(calibrate_cluster)
    cluster.calibrate_offsets();
  else
    LOG(ANABRID_DEBUG_CALIBRATION, "Skipping error-prone cluster calibration...");
  delay(100);

  // Start with a negative input offset and increase until we hit/cross zero
  for (auto mul_idx = 0u; mul_idx < NUM_MULTIPLIERS; mul_idx++) {
    if (!hardware->write_calibration_input_offsets(mul_idx, -0.1f, 0.0f))
      return utils::status("mblock write_calibration_input_offsets failed for (mul_idx=%d, -0.1f, 0.0f)", mul_idx);
    calibration[mul_idx].offset_x = -0.1f;
    for(;;) {
       auto measured = daq_->sample(mul_idx);
       if(measured >= 0.0) {
        if(calibration[mul_idx].offset_x >= 0.1) {
          LOGMEV("MBlock calibration mul_idx=%d at limit for offset_x=%f measured=%f",
             mul_idx, calibration[mul_idx].offset_x, measured
          );
          break; // either raise an error here or continue "silently"
        }
        if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x, 0.0f))
          return utils::status("mblock write_to_calibration for offset_x=%f failed.", calibration[mul_idx].offset_x);
        calibration[mul_idx].offset_x += 0.01f;
        delay(7);
       } else {
          LOGMEV("MBlock calibration mul_idx=%d completed for offset_x=%f measured=%f",
             mul_idx,calibration[mul_idx].offset_x, measured
          );
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
  if(calibrate_cluster)
    cluster.calibrate_offsets();
  else
    LOG(ANABRID_DEBUG_CALIBRATION, "Skipping error-prone cluster calibration...");
  delay(100);

  // Start with a negative input offset and increase until we hit/cross zero
  for (auto mul_idx = 0u; mul_idx < NUM_MULTIPLIERS; mul_idx++) {
    if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x, -0.1f))
      return utils::status("mblock write_calibration_input_offsets failed for (mul_idx=%d, %f, -0.1f)", mul_idx, calibration[mul_idx].offset_x);
    calibration[mul_idx].offset_y = -0.1f;
    for(;;) {
      auto measured = daq_->sample(mul_idx);
      if(measured >= 0.0) {
        if(calibration[mul_idx].offset_y >= 0.1) {
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
          LOGMEV("MBlock calibration mul_idx=%d completed for offset_y=%f measured=%f with offset_x=%f",
             mul_idx, calibration[mul_idx].offset_y, measured, calibration[mul_idx].offset_x);
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
         f_calibration_dac_0.set_channel(idx * 2,     (offset_y - 0.1f) * -12.5f);
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

bool blocks::EmptyMBlock::write_to_hardware() { return true; }

utils::status blocks::EmptyMBlock::config_self_from_json(JsonObjectConst cfg) {
  return utils::status::success();
}

uint8_t blocks::EmptyMBlock::get_entity_type() const { return static_cast<uint8_t>(MBlock::TYPES::UNKNOWN); }
