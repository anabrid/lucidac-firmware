// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/cblock.h"

#include "utils/logging.h"

FLASHMEM blocks::CBlock::CBlock(const bus::addr_t block_address, CBlockHAL *hardware)
    : FunctionBlock("C", block_address), hardware(hardware) {}

FLASHMEM blocks::CBlock::CBlock() : CBlock(bus::NULL_ADDRESS, new CBlockHALDummy()) {}

FLASHMEM float blocks::CBlock::get_factor(uint8_t idx) {
  if (idx >= NUM_COEFF)
    return 0.0f;
  return factors_[idx];
}

FLASHMEM const std::array<float, blocks::CBlock::NUM_COEFF> &blocks::CBlock::get_factors() const { return factors_; }

FLASHMEM bool blocks::CBlock::set_factor(uint8_t idx, float factor) {
  if (idx >= NUM_COEFF)
    return false;
  if (factor > MAX_FACTOR or factor < MIN_FACTOR)
    return false;

  factors_[idx] = factor;
  return true;
}

FLASHMEM void blocks::CBlock::set_factors(const std::array<float, NUM_COEFF> &factors) { factors_ = factors; }

FLASHMEM utils::status blocks::CBlock::write_to_hardware() {
  if (!write_factors_to_hardware()) {
    LOG(ANABRID_PEDANTIC, __PRETTY_FUNCTION__);
    return utils::status::failure();
  }
  return utils::status::success();
}

FLASHMEM bool blocks::CBlock::write_factors_to_hardware() {
  for (size_t i = 0; i < factors_.size(); i++) {
    if (!hardware->write_factor(i, factors_[i] * gain_corrections_[i]))
      return false;
  }
  return true;
}

FLASHMEM void blocks::CBlock::reset(bool keep_calibration) {
  FunctionBlock::reset(keep_calibration);
  for (size_t i = 0; i < NUM_COEFF; i++) {
    (void)set_factor(i, 1.0f);
  }
  if (!keep_calibration)
    reset_gain_corrections();
}

FLASHMEM float blocks::CBlock::get_gain_correction(uint8_t idx) const {
  if (idx > NUM_COEFF)
    return -2.0f;
  return gain_corrections_[idx];
}

FLASHMEM const std::array<float, blocks::CBlock::NUM_COEFF> &blocks::CBlock::get_gain_corrections() const {
  return gain_corrections_;
}

FLASHMEM void blocks::CBlock::reset_gain_corrections() {
  std::fill(gain_corrections_.begin(), gain_corrections_.end(), 1.0f);
}

FLASHMEM void blocks::CBlock::set_gain_corrections(const std::array<float, NUM_COEFF> &corrections) {
  gain_corrections_ = corrections;
};

FLASHMEM bool blocks::CBlock::set_gain_correction(const uint8_t coeff_idx, const float correction) {
  if (coeff_idx > NUM_COEFF)
    return false;
  // Gain correction must be positive and close to 1
  if (fabs(1.0f - correction) > MAX_GAIN_CORRECTION_ABS)
    return false;
  gain_corrections_[coeff_idx] = correction;
  return true;
};

FLASHMEM utils::status blocks::CBlock::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  for (auto cfgItr = cfg.begin(); cfgItr != cfg.end(); ++cfgItr) {
    if (cfgItr->key() == "elements") {
      auto res = _config_elements_form_json(cfgItr->value());
      if(!res) return res;
    } else {
      return utils::status("CBlock: Unknown configuration key");
    }
  }
  return utils::status::success();
}

FLASHMEM utils::status blocks::CBlock::_config_elements_form_json(const JsonVariantConst &cfg) {
  // Handle an array of factors
  if (cfg.is<JsonArrayConst>()) {
    auto factors = cfg.as<JsonArrayConst>();
    if (factors.size() != NUM_COEFF)
      return utils::status("Expecting %d elements in CBlock", NUM_COEFF);
    uint8_t idx = 0;
    for (JsonVariantConst factor : factors) {
      if (!factor.is<float>())
        return utils::status("CBlock: Cannot convert '%s' to float", factor.as<const char*>());
      if (!set_factor(idx++, factor.as<float>()))
        return utils::status("CBlock factor %f is out of valid bounds", factor.as<float>());
    }
    return utils::status::success();
  }
  // Handle a mapping of factors
  else if (cfg.is<JsonObjectConst>()) {
    serializeJson(cfg, Serial); //! I think this is debug code?
    for (JsonPairConst keyval : cfg.as<JsonObjectConst>()) {
      // Keys define index of factor to change
      // TODO: Check conversion from string to number
      auto idx = std::stoul(keyval.key().c_str());
      // Values can either be direct factor float values or {"factor": 0.42} objects
      if (keyval.value().is<JsonObjectConst>() and
          keyval.value().as<JsonObjectConst>().containsKey("factor")) {
        if (!set_factor(idx, keyval.value().as<JsonObjectConst>()["factor"].as<float>()))
          return utils::status("CBlock factor value is not a float or not within range");
      } else if (keyval.value().is<float>()) {
        if (!set_factor(idx, keyval.value().as<float>()))
          return utils::status("CBlock factor value is not within range");
      } else {
        return utils::status("CBlock factor value is not a float");
      }
    }
    return utils::status::success();
  }
  return utils::status("CBlock configuration must be an object or array.");
}

FLASHMEM void blocks::CBlock::config_self_to_json(JsonObject &cfg) {
  Entity::config_self_to_json(cfg);
  auto factors_cfg = cfg.createNestedArray("elements");
  for (auto idx = 0u; idx < factors_.size(); idx++) {
    factors_cfg.add(get_factor(idx));
  }
}

FLASHMEM blocks::CBlock *blocks::CBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_ or classifier.type != TYPE)
    return nullptr;

  if (classifier.version < entities::Version(1, 0))
    return nullptr;
  if (classifier.version < entities::Version(1, 1))
    return new CBlock(block_address, new CBlockHAL_V_1_0_X(block_address));
  if (classifier.version < entities::Version(1, 2))
    return new CBlock(block_address, new CBlockHAL_V_1_1_X(block_address));

  // Any unknown versuin results in a nullptr here.
  return nullptr;
}

std::array<const functions::AD5452, 32>
FLASHMEM blocks::CBlockHAL_Common::make_f_coeffs(bus::addr_t block_address, std::array<const uint8_t, 32> f_coeffs_cs) {
  return {functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[0])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[1])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[2])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[3])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[4])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[5])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[6])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[7])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[8])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[9])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[10])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[11])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[12])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[13])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[14])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[15])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[16])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[17])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[18])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[19])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[20])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[21])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[22])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[23])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[24])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[25])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[26])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[27])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[28])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[29])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[30])),
          functions::AD5452(bus::replace_function_idx(block_address, f_coeffs_cs[31]))};
}

FLASHMEM blocks::CBlockHAL_Common::CBlockHAL_Common(bus::addr_t block_address,
                                           std::array<const uint8_t, 32> f_coeffs_cs)
    : f_coeffs(make_f_coeffs(block_address, f_coeffs_cs)) {}

FLASHMEM bool blocks::CBlockHAL_Common::write_factor(uint8_t idx, float value) {
  if (idx >= 32)
    return false;
  // NOTE: The current hardware does not allow any error detection here.
  f_coeffs[idx].set_scale(value);
  return true;
}

FLASHMEM blocks::CBlockHAL_V_1_1_X::CBlockHAL_V_1_1_X(bus::addr_t block_address)
    : CBlockHAL_Common(block_address, {1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15, 16,
                                       17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32}) {}

FLASHMEM blocks::CBlockHAL_V_1_0_X::CBlockHAL_V_1_0_X(bus::addr_t block_address)
    : CBlockHAL_Common(block_address, {1,      2,       3,       4,       5,       6,       7,       8,
                                       9,      10,      11,      12,      13,      14,      15,      32 + 0,
                                       32 + 1, 32 + 2,  32 + 3,  32 + 4,  32 + 5,  32 + 6,  32 + 7,  32 + 8,
                                       32 + 9, 32 + 10, 32 + 11, 32 + 12, 32 + 13, 32 + 14, 32 + 15, 16}) {}
