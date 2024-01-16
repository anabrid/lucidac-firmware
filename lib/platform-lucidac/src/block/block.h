// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "block/base.h"
#include "bus/functions.h"
#include "bus/bus.h"

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

} // namespace blocks