// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/mblock.h"
#include "utils/logging.h"

#include "carrier/cluster.h"

#include "io/io.h" // Just for testing TODO REMOVE

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

blocks::MIntBlock::MIntBlock(const bus::addr_t block_address)
    : blocks::MBlock{block_address}, f_ic_dac(bus::replace_function_idx(block_address, IC_FUNC_IDX)),
      f_time_factor(bus::replace_function_idx(block_address, TIME_FACTOR_FUNC_IDX), true),
      f_time_factor_sync(bus::replace_function_idx(block_address, TIME_FACTOR_SYNC_FUNC_IDX)), ic_raw{0} {
  // Copying solves a strange linker issue "relocation against ... in read-only section `.text'"
  // TODO: Investigate problem further, replace by non-ugly solution
  auto default_ = DEFAULT_TIME_FACTOR;
  std::fill(std::begin(time_factors), std::end(time_factors), default_);
}

bool blocks::MIntBlock::set_ic(uint8_t idx, float value) {
  if (idx >= ic_raw.size())
    return false;
  if (value > 1.0f)
    value = 1.0f;
  if (value < -1.0f)
    value = -1.0f;
  ic_raw[idx] = decltype(f_ic_dac)::float_to_raw(value);
  return true;
}

bool blocks::MIntBlock::write_to_hardware() {
  for (decltype(ic_raw.size()) i = 0; i < ic_raw.size(); i++) {
    if (!f_ic_dac.set_channel(i, ic_raw[i])) {
      LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
      return false;
    }
  }
  return write_time_factors_to_hardware();
}

bool blocks::MIntBlock::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  f_ic_dac.init();
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

bool blocks::MIntBlock::write_time_factors_to_hardware() {
  uint8_t switches = 0;
  for (size_t index = 0; index < time_factors.size(); index++) {
    if (time_factors[index] != DEFAULT_TIME_FACTOR) {
      switches |= 1 << index;
    }
  }
  if (!f_time_factor.transfer8(switches))
    return false;

  f_time_factor_sync.trigger();
  return true;
}

bool blocks::MIntBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  if (cfg.containsKey("elements")) {
    // Handle an array of integrator configurations
    if (cfg["elements"].is<JsonArrayConst>()) {
      auto ints_cfg = cfg["elements"].as<JsonArrayConst>();
      if (ints_cfg.size() != NUM_INTEGRATORS) {
        return false;
      }
      for (size_t i = 0; i < ints_cfg.size(); i++) {
        if (!ints_cfg[i].containsKey("ic") or !ints_cfg[i]["ic"].is<float>())
          return false;
        if (!set_ic(i, ints_cfg[i]["ic"]))
          return false;
        if (ints_cfg[i].containsKey("k")) {
          if (!ints_cfg[i]["k"].is<unsigned int>())
            return false;
          if (!set_time_factor(i, ints_cfg[i]["k"]))
            return false;
        }
      }
    } else if (cfg["elements"].is<JsonObjectConst>()) {
      for (JsonPairConst keyval : cfg["elements"].as<JsonObjectConst>()) {
        // Value is an object, with any key being optional
        if (!keyval.value().is<JsonObjectConst>())
          return false;
        // TODO: Check conversion from string to number
        auto int_idx = std::stoul(keyval.key().c_str());
        if (int_idx >= NUM_INTEGRATORS)
          return false;
        auto int_cfg = keyval.value().as<JsonObjectConst>();
        if (int_cfg.containsKey("ic")) {
          if (!int_cfg["ic"].is<float>())
            return false;
          if (!set_ic(int_idx, int_cfg["ic"]))
            return false;
        }
        if (int_cfg.containsKey("k")) {
          if (!int_cfg["k"].is<unsigned int>())
            return false;
          if (!set_time_factor(int_idx, int_cfg["k"]))
            return false;
        }
      }
    } else {
      return false;
    }
  }
  return true;
}

void blocks::MIntBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  auto ints_cfg = cfg.createNestedArray("elements");
  for (size_t i = 0; i < NUM_INTEGRATORS; i++) {
    auto int_cfg = ints_cfg.createNestedObject();
    int_cfg["ic"] = decltype(f_ic_dac)::raw_to_float(ic_raw[i]);
    int_cfg["k"] = time_factors[i];
  }
}

bool blocks::MMulBlock::write_to_hardware() { return true; }

bool blocks::MMulBlock::config_self_from_json(JsonObjectConst cfg) {
  // MMulBlock does not expect any configuration currently.
  // But due to automation, some may still be sent.
  // Thus we accept any configuration containing only empty values or similar.
  if (cfg.containsKey("elements")) {
    if (cfg["elements"].is<JsonObjectConst>()) {
      // TODO: Implement
      return false;
    } else if (cfg["elements"].is<JsonArrayConst>()) {
      auto elements_cfg = cfg["elements"].as<JsonArrayConst>();
      if (elements_cfg.size() != NUM_MULTIPLIERS) {
        return false;
      }
      // TODO: Check each element. But currently makes no sense
      // for (const auto& element_cfg : elements_cfg) {
      //}
    } else {
      return false;
    }
  }
  return true;
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

  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new MIntBlock(block_address);
}

blocks::MMulBlock *blocks::MMulBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                             const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != static_cast<uint8_t>(TYPE))
    return nullptr;

  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new MMulBlock(block_address, new MMulBlockHAL_V_1_0_1(block_address));
}

blocks::MMulBlock::MMulBlock(bus::addr_t block_address, MMulBlockHAL *hardware)
    : MBlock(block_address), hardware(hardware) {}

bool blocks::MMulBlock::init() {
  // TODO: Remove once hardware pointer is part of FunctionBlock base class
  return FunctionBlock::init() and hardware->init();
}

bool blocks::MMulBlock::calibrate(daq::BaseDAQ *daq_, carrier::Carrier &carrier_, platform::Cluster &cluster) {
  LOG(ANABRID_DEBUG_CALIBRATION, __PRETTY_FUNCTION__);
  if (!MBlock::calibrate(daq_, carrier_, cluster))
    return false;

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
  cluster.calibrate_offsets();

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
  cluster.calibrate_offsets();
  delay(100);

  // Start with a negative input offset and increase until we hit/cross zero
  for (auto mul_idx = 0u; mul_idx < NUM_MULTIPLIERS; mul_idx++) {
    if (!hardware->write_calibration_input_offsets(mul_idx, -0.1f, 0.0f))
      return false;
    calibration[mul_idx].offset_x = -0.1f;
    while (daq_->sample(mul_idx) < 0.0f) {
      if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x, 0.0f))
        return false;
      calibration[mul_idx].offset_x += 0.01f;
      delay(300);
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
  cluster.calibrate_offsets();
  delay(100);

  // Start with a negative input offset and increase until we hit/cross zero
  for (auto mul_idx = 0u; mul_idx < NUM_MULTIPLIERS; mul_idx++) {
    if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x, -0.1f))
      return false;
    calibration[mul_idx].offset_y = -0.1f;
    while (daq_->sample(mul_idx) < 0.0f) {
      if (!hardware->write_calibration_input_offsets(mul_idx, calibration[mul_idx].offset_x,
                                                     calibration[mul_idx].offset_y))
        return false;
      calibration[mul_idx].offset_y += 0.01f;
      delay(300);
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

bool blocks::MMulBlockHAL_V_1_0_1::init() {
  return f_calibration_dac_0.init() and f_calibration_dac_0.set_external_reference() and
         f_calibration_dac_0.set_double_gain() and f_calibration_dac_1.init() and
         f_calibration_dac_1.set_external_reference() and f_calibration_dac_1.set_double_gain() and
         MMulBlockHAL::init();
}

blocks::MMulBlockHAL_V_1_0_1::MMulBlockHAL_V_1_0_1(bus::addr_t block_address)
    : f_calibration_dac_0(bus::address_from_tuple(block_address, 4)),
      f_calibration_dac_1(bus::address_from_tuple(block_address, 5)) {}

bool blocks::MMulBlockHAL_V_1_0_1::write_calibration_input_offsets(uint8_t idx, float offset_x,
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

bool blocks::MMulBlockHAL_V_1_0_1::reset_calibration_input_offsets() {
  for (auto idx = 0u; idx < MMulBlock::NUM_MULTIPLIERS; idx++)
    if (!write_calibration_input_offsets(idx, 0, 0))
      return false;
  return true;
}

bool blocks::MMulBlockHAL_V_1_0_1::reset_calibration_output_offsets() {
  for (auto idx = 0u; idx < MMulBlock::NUM_MULTIPLIERS; idx++)
    if (!write_calibration_output_offset(idx, 0))
      return false;
  return true;
}

bool blocks::MMulBlockHAL_V_1_0_1::write_calibration_output_offset(uint8_t idx, float offset_z) {
  // See write_calibration_input_offsets for some explanations.
  if (fabs(offset_z) > 0.1f)
    return false;
  return f_calibration_dac_1.set_channel(idx, (offset_z - 0.1f) * -12.5f);
}

bool blocks::EmptyMBlock::write_to_hardware() { return true; }

bool blocks::EmptyMBlock::config_self_from_json(JsonObjectConst cfg) { return true; }

uint8_t blocks::EmptyMBlock::get_entity_type() const { return static_cast<uint8_t>(MBlock::TYPES::UNKNOWN); }
