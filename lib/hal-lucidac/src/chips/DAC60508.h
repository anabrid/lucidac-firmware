// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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

  static constexpr uint8_t REG_DAC(const uint8_t i) { return 8 + i; };

  static constexpr uint16_t RAW_ZERO = 0x0;
  static constexpr uint16_t RAW_TWO_FIVE = 0xFFF0;

  static const SPISettings DEFAULT_SPI_SETTINGS;
  static uint16_t float_to_raw(float value);
  static float raw_to_float(uint16_t value);

  using DataFunction::DataFunction;
  explicit DAC60508(bus::addr_t address);

  bool set_channel_raw(uint8_t idx, uint16_t value) const;
  bool set_channel(uint8_t idx, float value) const;
  bool init() const;

private:
  uint16_t read_register(uint8_t address) const;
  bool write_register(uint8_t address, uint16_t data) const;
};

} // namespace functions
