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
  static constexpr uint8_t BLOCK_IDX = bus::C_BLOCK_IDX;

  static constexpr uint8_t COEFF_BASE_FUNC_IDX = 1;

  static constexpr uint8_t NUM_COEFF = 32;

  static constexpr std::array<uint8_t, NUM_COEFF> INPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

  static constexpr std::array<uint8_t, NUM_COEFF> OUTPUT_IDX_RANGE() {
    return {0,  1,  2,  3,  4,  5,  6,  7,  8,  9,  10, 11, 12, 13, 14, 15,
            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31};
  };

protected:
  std::array<const functions::AD5452, NUM_COEFF> f_coeffs;

  std::array<uint16_t, NUM_COEFF> factors_{{0}};

 [[nodiscard]] bool write_factors_to_hardware();

public:
  explicit CBlock(uint8_t clusterIdx);

  bus::addr_t get_block_address() override;

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
  [[nodiscard]] bool set_factor(uint8_t idx, float factor);
  float get_factor(uint8_t idx);

  [[nodiscard]] bool write_to_hardware() override;
  void reset(bool keep_calibration) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

protected:
  void config_self_to_json(JsonObject &cfg) override;
};

} // namespace blocks