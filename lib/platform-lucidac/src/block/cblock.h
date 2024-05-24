// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "block/base.h"
#include "bus/bus.h"
#include "chips/AD5452.h"
#include "chips/SR74HCT595.h"

namespace blocks {

// ██████   █████  ███████ ███████      ██████ ██       █████  ███████ ███████
// ██   ██ ██   ██ ██      ██          ██      ██      ██   ██ ██      ██
// ██████  ███████ ███████ █████       ██      ██      ███████ ███████ ███████
// ██   ██ ██   ██      ██ ██          ██      ██      ██   ██      ██      ██
// ██████  ██   ██ ███████ ███████      ██████ ███████ ██   ██ ███████ ███████

/**
 * The Lucidac Coefficient Block (C-Block) is represented by this class.
 *
 * This class provides a neat interface for setting digital potentiometers
 * without having to worry for "raw" DPT values or upscaling factors
 * (allowing the DPTs to take values between [-20,+20] instead of only
 * [-1,+1]).
 *
 * As a Lucidac can only have a single C-Block, this is kind of a singleton.
 * Typical usage happens via the Lucidac class.
 *
 **/
class CBlock : public FunctionBlock {
public:
  // Entity hardware identifier information.
  // For CBlocks, there is only one entity type possible currently.
  static constexpr auto CLASS_ = entities::EntityClass::C_BLOCK;
  static constexpr uint8_t TYPE = 1;
  enum class VARIANTS : uint8_t { UNKNOWN = 0, SEQUENTIAL_ADDRESSES = 1, MIXED_ADDRESSES = 2 };

  static CBlock *from_entity_classifier(entities::EntityClassifier classifier, bus::addr_t block_address);

public:
  static constexpr uint8_t BLOCK_IDX = bus::C_BLOCK_IDX;

  static constexpr uint8_t COEFF_BASE_FUNC_IDX = 1;
  static constexpr uint8_t SCALE_SWITCHER = 33;
  static constexpr uint8_t SCALE_SWITCHER_SYNC = 34;
  static constexpr uint8_t SCALE_SWITCHER_CLEAR = 35;

  static constexpr uint8_t NUM_COEFF = 32;
  static constexpr float MAX_REAL_FACTOR = 2.0f;
  static constexpr float MIN_REAL_FACTOR = -2.0f;
  // TODO: Upscaling is not *exactly* 10
  static constexpr float UPSCALING = 10.055f;
  static constexpr float MAX_FACTOR = MAX_REAL_FACTOR * UPSCALING;
  static constexpr float MIN_FACTOR = MIN_REAL_FACTOR * UPSCALING;

protected:
  std::array<const functions::AD5452, NUM_COEFF> f_coeffs;
  const functions::SR74HCT595 f_upscaling;
  const functions::TriggerFunction f_upscaling_sync;
  const functions::TriggerFunction f_upscaling_clear;

  std::array<uint16_t, NUM_COEFF> factors_{{0}};
  uint32_t upscaling_{0};

  void set_upscaling(uint8_t idx, bool enable);
  bool is_upscaled(uint8_t idx);

  void write_factors_to_hardware();
  void write_upscaling_to_hardware();

  CBlock(bus::addr_t block_address, std::array<const functions::AD5452, NUM_COEFF> fCoeffs,
         functions::SR74HCT595 fUpscaling, functions::TriggerFunction fUpscalingSync,
         functions::TriggerFunction fUpscalingClear);

public:
  explicit CBlock(bus::addr_t block_address);
  CBlock();

  entities::EntityClass get_entity_class() const final { return entities::EntityClass::C_BLOCK; }

  // Variants exist, force them to differentiate themselves
  uint8_t get_entity_variant() const override = 0;

  /**
   * Set a particular digital potentiometer.
   *
   * idx means the coefficient ID and is between [0,31].
   * factor is the actual analog value between [-20,20].
   *
   * Note that calling this function only stores the value
   * in the in-memory representation of the hybrid controller.
   * The in-memory representation is flushed to hardware by
   * calling write_to_hardware().
   *
   * @returns false in case of invalid input, true else.
   **/
  bool set_factor(uint8_t idx, float factor);
  float get_factor(uint8_t idx);

  void write_to_hardware() override;
  void reset(bool keep_calibration) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

//  ██    ██  █████  ██████  ██  █████  ███    ██ ████████ ███████
//  ██    ██ ██   ██ ██   ██ ██ ██   ██ ████   ██    ██    ██
//  ██    ██ ███████ ██████  ██ ███████ ██ ██  ██    ██    ███████
//   ██  ██  ██   ██ ██   ██ ██ ██   ██ ██  ██ ██    ██         ██
//    ████   ██   ██ ██   ██ ██ ██   ██ ██   ████    ██    ███████

class CBlock_SequentialAddresses : public CBlock {
public:
  explicit CBlock_SequentialAddresses(bus::addr_t block_address)
      : CBlock(block_address,
               {
                   functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 0),
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
                   functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31),
               },
               functions::SR74HCT595{bus::replace_function_idx(block_address, SCALE_SWITCHER)},
               functions::TriggerFunction{bus::replace_function_idx(block_address, SCALE_SWITCHER_SYNC)},
               functions::TriggerFunction{bus::replace_function_idx(block_address, SCALE_SWITCHER_CLEAR)}) {}

  CBlock_SequentialAddresses() : CBlock_SequentialAddresses(bus::idx_to_addr(0, BLOCK_IDX, 0)) {}

  uint8_t get_entity_variant() const final {
    return static_cast<uint8_t>(CBlock::VARIANTS::SEQUENTIAL_ADDRESSES);
  }
};

class CBlock_MixedAddresses : public CBlock {
public:
  explicit CBlock_MixedAddresses(const bus::addr_t block_address)
      : CBlock(block_address,
               {functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 0),
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
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 0),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 1),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 2),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 3),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 4),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 5),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 6),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 7),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 8),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 9),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 10),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 11),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 12),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 13),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 14),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 31 + 15),
                functions::AD5452(bus::replace_function_idx(block_address, COEFF_BASE_FUNC_IDX), 16)},
               // TODO: Dynamic hardware detection is implemented for both REV0 as well as REV1,
               //       but functions for scale switching have been removed in REV1.
               functions::SR74HCT595(bus::NULL_ADDRESS), functions::TriggerFunction(bus::NULL_ADDRESS),
               functions::TriggerFunction(bus::NULL_ADDRESS)) {}

  CBlock_MixedAddresses() : CBlock_MixedAddresses(bus::idx_to_addr(0, BLOCK_IDX, 0)) {}

  uint8_t get_entity_variant() const final { return static_cast<uint8_t>(CBlock::VARIANTS::MIXED_ADDRESSES); }
};

} // namespace blocks
