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

#include <array>
#include <cstdint>

#include "base_block.h"
#include "functions.h"
#include "local_bus.h"

namespace blocks {

class CScaleSwitchFunction : public functions::_old_DataFunction {
public:
  //! Bits sent to the shift register, least significant bit is SW.0, most significant bit is SW.35.
  uint32_t data = 0;

  using functions::_old_DataFunction::_old_DataFunction;
  explicit CScaleSwitchFunction(bus::addr_t address);

  void write_to_hardware() const;
};

class CCoeffFunction : public functions::_old_DataFunction {
public:
  uint16_t data = 0;

  using functions::_old_DataFunction::_old_DataFunction;
  CCoeffFunction(bus::addr_t base_address, uint8_t coeff_idx);

  void write_to_hardware() const;
};

class CBlock : public FunctionBlock {
public:
  static constexpr uint8_t COEFF_BASE_FUNC_IDX = 1;
  static constexpr uint8_t SCALE_SWITCHER = 33;
  static constexpr uint8_t SCALE_SWITCHER_SYNC = 34;
  static constexpr uint8_t SCALE_SWITCHER_CLEAR = 35;

  using FunctionBlock::FunctionBlock;
};

} // namespace blocks