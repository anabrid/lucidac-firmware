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

#pragma once

#include <cstdint>

#include "DAC60508.h"
#include "SR74HCT595.h"
#include "base_block.h"
#include "local_bus.h"

namespace blocks {

/**
 * A Lucidac Math block (M-Block) is represented by this class.
 * Lucidac currently has two Math blocks, referred to as M1 and M2.
 * Both are represented by instances of this class or a suitable subclass.
 *
 * This base class provides convenient static functions to compute the correct
 * index (for usage in the U and I block) for M block computing elements.
 *
 **/
class MBlock : public FunctionBlock {
public:
  static constexpr uint8_t M1_IDX = bus::M1_BLOCK_IDX;
  static constexpr uint8_t M2_IDX = bus::M2_BLOCK_IDX;

  //! M1 input signal specifier for hard-coded usage, like MBlock::M1_INPUT<3>().
  template <int n> static constexpr uint8_t M1_INPUT() {
    static_assert(n < 8, "MBlock input must be less than 8.");
    return n + 8;
  }

  //! M1 input signal specifier for dynamic usage, like MBlock::M1_INPUT(variable).
  static constexpr uint8_t M1_INPUT(uint8_t idx) { return idx + 8; }

  //! M1 output signal specifier for hard-coded usage, like MBlock::M1_OUTPUT<3>().
  template <int n> static constexpr uint8_t M1_OUTPUT() {
    static_assert(n < 8, "MBlock output must be less than 8.");
    return n + 8;
  }

  //! M1 output signal specifier for dynamic usage, like MBlock::M1_OUTPUT(variable).
  static constexpr uint8_t M1_OUTPUT(uint8_t idx) { return idx + 8; }

  //! M2 input signal specifier for hard-coded usage, like MBlock::M2_INPUT<3>().
  template <int n> static constexpr uint8_t M2_INPUT() {
    static_assert(n < 8, "MBlock input must be less than 8.");
    return n;
  }

  //! M2 input signal specifier for dynamic usage, like MBlock::M2_INPUT(variable).
  static constexpr uint8_t M2_INPUT(uint8_t idx) { return idx; }

  //! M2 output signal specifier for hard-coded usage, like MBlock::M2_OUTPUT<3>().
  template <int n> static constexpr uint8_t M2_OUTPUT() {
    static_assert(n < 8, "MBlock output must be less than 8.");
    return n;
  }

  //! M2 output signal specifier for dynamic usage, like MBlock::M2_OUTPUT(variable).
  static constexpr uint8_t M2_OUTPUT(uint8_t idx) { return idx; }

protected:
  uint8_t slot_idx;

public:
  MBlock(uint8_t cluster_idx, uint8_t slot_idx);

  bus::addr_t get_block_address() override;
};

// HINT: Consider renaming this MBlockInt
//       in particular if we get some MBlockMult (MMultBlock reads weird)
class MIntBlock : public MBlock {
public:
  static constexpr uint8_t IC_FUNC_IDX = 1;
  static constexpr uint8_t TIME_FACTOR_FUNC_IDX = 3;
  static constexpr uint8_t TIME_FACTOR_SYNC_FUNC_IDX = 4;
  static constexpr uint8_t TIME_FACTOR_RESET_FUNC_IDX = 5;

  static constexpr unsigned int DEFAULT_TIME_FACTOR = 10000;

private:
  functions::DAC60508 f_ic_dac;
  ::functions::SR74HCT595 f_time_factor;
  ::functions::TriggerFunction f_time_factor_sync;
  std::array<uint16_t, 8> ic_raw;
  std::array<unsigned int, 8> time_factors;

  void write_time_factors_to_hardware();

public:
  MIntBlock(uint8_t cluster_idx, uint8_t slot_idx);

  bool init() override;

  bool set_ic(uint8_t idx, float value);

  bool set_time_factor(uint8_t int_idx, unsigned int k);

  void write_to_hardware() override;
};

} // namespace blocks