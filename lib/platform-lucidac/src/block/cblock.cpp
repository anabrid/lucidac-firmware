// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include "cblock.h"

blocks::CBlock::CBlock(uint8_t clusterIdx)
    : FunctionBlock("C", clusterIdx),
      f_coeffs{
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 0),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 1),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 2),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 3),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 4),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 5),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 6),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 7),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 8),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 9),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 10),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 11),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 12),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 13),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 14),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 15),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 16),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 17),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 18),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 19),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 20),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 21),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 22),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 23),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 24),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 25),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 26),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 27),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 28),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 29),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 30),
          functions::AD5452(bus::idx_to_addr(clusterIdx, BLOCK_IDX, COEFF_BASE_FUNC_IDX), 31),
      },
      f_upscaling(bus::idx_to_addr(clusterIdx, BLOCK_IDX, SCALE_SWITCHER)),
      f_upscaling_sync(bus::idx_to_addr(clusterIdx, BLOCK_IDX, SCALE_SWITCHER_SYNC)),
      f_upscaling_clear(bus::idx_to_addr(clusterIdx, BLOCK_IDX, SCALE_SWITCHER_CLEAR)) {}

bus::addr_t blocks::CBlock::get_block_address() { return bus::idx_to_addr(cluster_idx, BLOCK_IDX, 0); }

float blocks::CBlock::get_factor(uint8_t idx) {
  if (idx >= NUM_COEFF)
    return 0.0f;

  auto factor = decltype(f_coeffs)::value_type::raw_to_float(factors_[idx]);
  if (is_upscaled(idx))
    factor = factor * UPSCALING;
  return factor;
}

bool blocks::CBlock::set_factor(uint8_t idx, float factor) {
  if (idx >= NUM_COEFF)
    return false;
  if (factor > MAX_FACTOR)
    return false;
  if (factor < MIN_FACTOR)
    return false;
  if (factor > MAX_REAL_FACTOR or factor < MIN_REAL_FACTOR) {
    set_upscaling(idx, true);
    factor = factor / UPSCALING;
  } else {
    set_upscaling(idx, false);
  }
  factors_[idx] = decltype(f_coeffs)::value_type::float_to_raw(factor);
  return true;
}

bool blocks::CBlock::is_upscaled(uint8_t idx) {
  if (idx >= NUM_COEFF)
    return false;
  return upscaling_ & (1 << idx);
}

void blocks::CBlock::set_upscaling(uint8_t idx, bool enable = true) {
  if (enable)
    upscaling_ |= 1 << idx;
  else
    upscaling_ &= ~(1 << idx);
}

void blocks::CBlock::write_to_hardware() {
  write_factors_to_hardware();
  write_upscaling_to_hardware();
}

void blocks::CBlock::write_factors_to_hardware() {
  for (size_t i = 0; i < f_coeffs.size(); i++) {
    f_coeffs[i].set_scale(factors_[i]);
  }
}

void blocks::CBlock::write_upscaling_to_hardware() {
  f_upscaling.transfer32(upscaling_);
  f_upscaling_sync.trigger();
}

void blocks::CBlock::reset(bool keep_calibration) {
  FunctionBlock::reset(keep_calibration);
  for (size_t i = 0; i < NUM_COEFF; i++) {
    set_factor(i, 0.f);
  }
}

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
        if (keyval.value().is<JsonObjectConst>() and keyval.value().as<JsonObjectConst>().containsKey("factor")) {
          if (!set_factor(idx, keyval.value().as<JsonObjectConst>()["factor"].as<float>()))
            return false;
        }
        else if (keyval.value().is<float>()) {
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
