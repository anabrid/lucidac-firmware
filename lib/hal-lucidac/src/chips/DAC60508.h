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

#include "bus/bus.h"
#include "bus/functions.h"

namespace functions {

class DAC60508 : public DataFunction {
public:
  static constexpr uint8_t REG_NOOP = 0;
  static constexpr uint8_t REG_DEVICE_ID = 1;
  static constexpr uint8_t REG_SYNC = 2;
  static constexpr uint8_t REG_CONFIG = 3;
  static constexpr uint8_t REG_GAIN = 4;
  static constexpr uint8_t REG_TRIGGER = 5;
  static constexpr uint8_t REG_BROADCAST = 6;
  static constexpr uint8_t REG_STATUS = 7;
  static constexpr uint8_t REG_DAC(const uint8_t i) {
    return 8 + i;
  };

  static constexpr uint16_t RAW_MINUS_ONE = 0x0;
  static constexpr uint16_t RAW_PLUS_ONE = 0xD1D0;

  static const SPISettings DEFAULT_SPI_SETTINGS;
  static uint16_t float_to_raw(float value);
  static float raw_to_float(uint16_t value);

  using DataFunction::DataFunction;
  explicit DAC60508(bus::addr_t address);

  uint16_t read_register(uint8_t address) const;
  bool write_register(uint8_t address, uint16_t data) const;
  bool set_channel(uint8_t idx, uint16_t value) const;
  void init() const;
};

} // namespace functions