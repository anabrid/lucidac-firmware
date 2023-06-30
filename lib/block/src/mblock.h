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
#include "base_block.h"
#include "local_bus.h"

namespace blocks {

/**
  * A Lucidac Math block (M-Block) is represented by this class.
  * Lucidac currently has two Math blocks, refered to as M1 and M2.
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

  static constexpr uint8_t M1_INPUT(uint8_t idx) { return idx + 8; }

  static constexpr uint8_t M1_OUTPUT(uint8_t idx) { return idx + 8; }

  static constexpr uint8_t M2_INPUT(uint8_t idx) { return idx; }

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

private:
  functions::DAC60508 f_ic_dac;
  std::array<uint16_t, 8> ic_raw;

public:
  MIntBlock(uint8_t cluster_idx, uint8_t slot_idx);

  bool init() override;

  bool set_ic(uint8_t idx, float value);

  void write_to_hardware() override;
};


} // namespace blocks