// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/mblock.h"
#include "utils/logging.h"

#include "carrier/cluster.h"
#include "mode/mode.h"

FLASHMEM blocks::MIntBlock::MIntBlock(bus::addr_t block_address, MIntBlockHAL *hardware)
    : blocks::MBlock{block_address}, hardware(hardware), ic_values{}, time_factors{} {
  reset_ic_values();
  reset_time_factors();
}

FLASHMEM bool blocks::MIntBlock::init() {
  if (!FunctionBlock::init())
    return false;
  if (!hardware->init())
    return false;
  return true;
}

FLASHMEM const std::array<float, 8> &blocks::MIntBlock::get_ic_values() const { return ic_values; }

FLASHMEM float blocks::MIntBlock::get_ic_value(uint8_t idx) const {
  if (idx >= ic_values.size())
    return 0.0f;
  return ic_values[idx];
}

FLASHMEM bool blocks::MIntBlock::set_ic_values(const std::array<float, 8> &ic_values_) {
  for (auto idx = 0u; idx < ic_values_.size(); idx++)
    if (!set_ic_value(idx, ic_values_[idx]))
      return false;
  return true;
}

FLASHMEM bool blocks::MIntBlock::set_ic_values(float value) {
  for (auto idx = 0u; idx < ic_values.size(); idx++)
    if (!set_ic_value(idx, value))
      return false;
  return true;
}

FLASHMEM bool blocks::MIntBlock::set_ic_value(uint8_t idx, float value) {
  if (idx >= ic_values.size())
    return false;
  if (value > 1.0f or value < -1.0f)
    return false;
  ic_values[idx] = value;
  return true;
}

FLASHMEM void blocks::MIntBlock::reset_ic_values() { std::fill(ic_values.begin(), ic_values.end(), 0.0f); }

FLASHMEM const std::array<unsigned int, 8> &blocks::MIntBlock::get_time_factors() const { return time_factors; }

FLASHMEM unsigned int blocks::MIntBlock::get_time_factor(uint8_t idx) const {
  if (idx >= time_factors.size())
    return 0;
  return time_factors[idx];
}

FLASHMEM bool blocks::MIntBlock::set_time_factors(const std::array<unsigned int, 8> &time_factors_) {
  for (auto idx = 0u; idx < time_factors_.size(); idx++)
    if (!set_time_factor(idx, time_factors_[idx]))
      return false;
  return true;
}

FLASHMEM bool blocks::MIntBlock::set_time_factors(unsigned int k) {
  for (auto idx = 0u; idx < time_factors.size(); idx++)
    if (!set_time_factor(idx, k))
      return false;
  return true;
}

FLASHMEM bool blocks::MIntBlock::set_time_factor(uint8_t int_idx, unsigned int k) {
  if (!(k == 100 or k == 10000))
    return false;
  if (int_idx >= NUM_INTEGRATORS)
    return false;
  time_factors[int_idx] = k;
  return true;
}

FLASHMEM void blocks::MIntBlock::reset_time_factors() {
  // Copying solves a strange linker issue "relocation against ... in read-only section `.text'"
  auto default_ = DEFAULT_TIME_FACTOR;
  std::fill(std::begin(time_factors), std::end(time_factors), default_);
}

FLASHMEM utils::status blocks::MIntBlock::write_to_hardware() {
  // Write IC values one channel at a time
  for (decltype(ic_values.size()) i = 0; i < ic_values.size(); i++) {
    if (!hardware->write_ic(i, ic_values[i])) {
      LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
      return utils::status::failure();
    }
  }
  // Write time factor switches by converting to bitset
  std::bitset<NUM_INTEGRATORS> time_factor_switches{};
  for (auto idx = 0u; idx < time_factors.size(); idx++)
    if (time_factors[idx] != DEFAULT_TIME_FACTOR)
      time_factor_switches.set(idx);
  if (!hardware->write_time_factor_switches(time_factor_switches))
    return utils::status::failure();
  return utils::status::success();
}

FLASHMEM void blocks::MIntBlock::reset(entities::ResetAction action) {
  FunctionBlock::reset(action);
  reset_ic_values();
  reset_time_factors();

  if (action.has(entities::ResetAction::OVERLOAD_RESET)) {
    // !!ATTENTION!! This breaks a lot of things but is neccessary and the ONLY
    // way to reset overloads on the MIntBlock.
    mode::RealManualControl::enable();
    mode::RealManualControl::to_ic();
  }
}

FLASHMEM utils::status blocks::MIntBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "elements") {
      auto res = _config_elements_from_json(cfgItr->value());
      if (!res)
        return res;
    } else {
      return utils::status("Unknown configuration key.");
    }
  }
  return utils::status::success();
}

FLASHMEM utils::status blocks::MIntBlock::_config_elements_from_json(const JsonVariantConst &cfg) {
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

FLASHMEM void blocks::MIntBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  auto ints_cfg = cfg.createNestedArray("elements");
  for (size_t i = 0; i < NUM_INTEGRATORS; i++) {
    auto int_cfg = ints_cfg.createNestedObject();
    int_cfg["ic"] = ic_values[i];
    int_cfg["k"] = time_factors[i];
  }
}

FLASHMEM blocks::MIntBlock *blocks::MIntBlock::from_entity_classifier(entities::EntityClassifier classifier,
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

// Hardware abstraction layer

FLASHMEM blocks::MIntBlockHAL_V_1_0_X::MIntBlockHAL_V_1_0_X(bus::addr_t block_address)
    : f_ic_dac(bus::replace_function_idx(block_address, 4)),
      f_time_factor(bus::replace_function_idx(block_address, 5), true),
      f_time_factor_sync(bus::replace_function_idx(block_address, 6)),
      f_time_factor_reset(bus::replace_function_idx(block_address, 7)) {}

FLASHMEM bool blocks::MIntBlockHAL_V_1_0_X::init() {
  if (!MIntBlockHAL::init())
    return false;
  return f_ic_dac.init() and f_ic_dac.set_external_reference(true) and f_ic_dac.set_double_gain(true);
}

FLASHMEM bool blocks::MIntBlockHAL_V_1_0_X::write_ic(uint8_t idx, float ic) {
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

FLASHMEM bool blocks::MIntBlockHAL_V_1_0_X::write_time_factor_switches(std::bitset<8> switches) {
  if (!f_time_factor.transfer8(static_cast<uint8_t>(switches.to_ulong())))
    return false;
  f_time_factor_sync.trigger();
  return true;
}
