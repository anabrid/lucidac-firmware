// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "mblock.h"
#include "logging.h"

blocks::MBlock::MBlock(bus::addr_t block_address)
    : blocks::FunctionBlock{std::string("M") + std::string(
                                                   // Addresses 4, 9, 14 are M0
                                                   // Addresses 5, 10, 15 are M1
                                                   block_address % 5 ? "0" : "1"),
                            block_address} {}

blocks::MIntBlock::MIntBlock(const bus::addr_t block_address)
    : blocks::MBlock{block_address}, f_ic_dac(bus::replace_function_idx(block_address, IC_FUNC_IDX)),
      f_time_factor(bus::replace_function_idx(block_address, TIME_FACTOR_FUNC_IDX), true),
      f_time_factor_sync(bus::replace_function_idx(block_address, TIME_FACTOR_SYNC_FUNC_IDX)), ic_raw{0} {
  std::fill(std::begin(time_factors), std::end(time_factors), DEFAULT_TIME_FACTOR);
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

void blocks::MIntBlock::write_to_hardware() {
  for (decltype(ic_raw.size()) i = 0; i < ic_raw.size(); i++) {
    f_ic_dac.set_channel(i, ic_raw[i]);
  }
  write_time_factors_to_hardware();
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

void blocks::MIntBlock::write_time_factors_to_hardware() {
  uint8_t switches = 0;
  for (size_t index = 0; index < time_factors.size(); index++) {
    if (time_factors[index] != DEFAULT_TIME_FACTOR) {
      switches |= 1 << index;
    }
  }
  f_time_factor.transfer(switches);
  f_time_factor_sync.trigger();
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

void blocks::MMulBlock::write_to_hardware() {}

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
  // Assume classifier has already been sanity checked
  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new MIntBlock(block_address);
}

blocks::MMulBlock *blocks::MMulBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                             const bus::addr_t block_address) {
  // Assume classifier has already been sanity checked
  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new MMulBlock(block_address);
}
