// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "cblock.h"

#include "logging.h"

blocks::CBlock::CBlock(const bus::addr_t block_address, std::array<const functions::AD5452, NUM_COEFF> fCoeffs)
    : FunctionBlock("C", block_address), f_coeffs(std::move(fCoeffs)) {}

blocks::CBlock::CBlock(const bus::addr_t block_address)
    : FunctionBlock("C", block_address),
      f_coeffs{functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 0),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 1),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 2),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 3),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 4),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 5),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 6),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 7),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 8),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 9),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 10),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 11),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 12),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 13),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 14),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 15),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 16),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 17),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 18),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 19),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 20),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 21),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 22),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 23),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 24),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 25),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 26),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 27),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 28),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 29),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 30),
               functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31)} {}

blocks::CBlock::CBlock() : CBlock(bus::idx_to_addr(0, bus::C_BLOCK_IDX, 0)) {}

float blocks::CBlock::get_factor(uint8_t idx) {
  if (idx >= NUM_COEFF)
    return 0.0f;

  auto factor = decltype(f_coeffs)::value_type::raw_to_float(factors_[idx]);
  return factor;
}

bool blocks::CBlock::set_factor(uint8_t idx, float factor) {
  if (idx >= NUM_COEFF)
    return false;
  if (factor > decltype(f_coeffs)::value_type::MAX_FACTOR or
      factor < decltype(f_coeffs)::value_type::MIN_FACTOR)
    return false;

  factors_[idx] = decltype(f_coeffs)::value_type::float_to_raw(factor);
  return true;
}

bool blocks::CBlock::write_to_hardware() {
  if (!write_factors_to_hardware()) {
    LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
    return false;
  }
  return true;
}

bool blocks::CBlock::write_factors_to_hardware() {
  for (size_t i = 0; i < f_coeffs.size(); i++) {
    f_coeffs[i].set_scale(decltype(f_coeffs)::value_type::raw_to_float(factors_[i]) * gain_corrections_[i]);
  }
  return true;
}

void blocks::CBlock::reset(bool keep_calibration) {
  FunctionBlock::reset(keep_calibration);
  for (size_t i = 0; i < NUM_COEFF; i++) {
    set_factor(i, 0.f);
  }
  if (!keep_calibration)
    reset_gain_corrections();
}

const std::array<float, blocks::CBlock::NUM_COEFF> &blocks::CBlock::get_gain_corrections() const {
  return gain_corrections_;
}

void blocks::CBlock::reset_gain_corrections() {
  std::fill(gain_corrections_.begin(), gain_corrections_.end(), 1.0f);
}

void blocks::CBlock::set_gain_corrections(const std::array<float, NUM_COEFF> &corrections) {
    gain_corrections_ = corrections;
};

bool blocks::CBlock::set_gain_correction(const uint8_t coeff_idx, const float correction) {
  if (coeff_idx > NUM_COEFF)
    return false;
  gain_corrections_[coeff_idx] = correction;
  return true;
};

bool blocks::CBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  if (cfg.containsKey("elements")) {
    // Handle an array of factors
    if (cfg["elements"].is<JsonArrayConst>()) {
      auto factors = cfg["elements"].as<JsonArrayConst>();
      if (factors.size() != NUM_COEFF)
        return false;
      uint8_t idx = 0;
      for (JsonVariantConst factor : factors) {
        if (!factor.is<float>()) {
          return false;
        }
        set_factor(idx++, factor.as<float>());
      }
    }
    // Handle a mapping of factors
    if (cfg["elements"].is<JsonObjectConst>()) {
      serializeJson(cfg, Serial);
      for (JsonPairConst keyval : cfg["elements"].as<JsonObjectConst>()) {
        // Keys define index of factor to change
        // TODO: Check conversion from string to number
        auto idx = std::stoul(keyval.key().c_str());
        // Values can either be direct factor float values or {"factor": 0.42} objects
        if (keyval.value().is<JsonObjectConst>() and
            keyval.value().as<JsonObjectConst>().containsKey("factor")) {
          if (!set_factor(idx, keyval.value().as<JsonObjectConst>()["factor"].as<float>()))
            return false;
        } else if (keyval.value().is<float>()) {
          if (!set_factor(idx, keyval.value().as<float>()))
            return false;
        } else {
          return false;
        }
      }
    }
  }

  // The combination of checks above must not ignore any valid config dictionary
  return true;
}

void blocks::CBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  auto factors_cfg = cfg.createNestedArray("elements");
  for (auto idx = 0; idx < factors_.size(); idx++) {
    factors_cfg.add(get_factor(idx));
  }
}

blocks::CBlock *blocks::CBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != TYPE)
    return nullptr;

  auto variant = classifier.variant_as<VARIANTS>();
  switch (variant) {
  case VARIANTS::UNKNOWN:
    return nullptr;
  case VARIANTS::SEQUENTIAL_ADDRESSES:
    // There are no different versions of this variant currently
    return new CBlock_SequentialAddresses(block_address);
  case VARIANTS::MIXED_ADDRESSES:
    // There are no different versions of this variant currently
    return new CBlock_MixedAddresses(block_address);
  }
  // Any unknown value results in a nullptr here.
  // Adding default case to switch suppresses warnings about missing cases.
  return nullptr;
}
