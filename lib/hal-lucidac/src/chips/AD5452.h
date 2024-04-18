// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/functions.h"

namespace functions {

class AD5452 : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  static constexpr uint16_t RAW_MIN = 0;
  static constexpr uint16_t RAW_ZERO = 2047 << 2;
  static constexpr uint16_t RAW_MAX = 4095 << 2;
  static constexpr uint16_t RAW_DELTA_ONE = 2047 << 2;

  static constexpr float MIN_FACTOR = -1.0f;
  static constexpr float MAX_FACTOR = +1.0f;

  using DataFunction::DataFunction;
  explicit AD5452(bus::addr_t address);
  AD5452(bus::addr_t base_addr, uint8_t func_addr_shift);

  void set_scale(uint16_t scale_raw) const;
  void set_scale(float scale) const;
  static uint16_t float_to_raw(float scale);
  static float raw_to_float(uint16_t raw);
};

}