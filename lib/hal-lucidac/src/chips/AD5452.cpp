// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "AD5452.h"

const SPISettings functions::AD5452::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE1};

functions::AD5452::AD5452(bus::addr_t address) : DataFunction(address, DEFAULT_SPI_SETTINGS) {}

functions::AD5452::AD5452(bus::addr_t base_addr, uint8_t func_addr_shift)
    : AD5452(bus::increase_function_idx(base_addr, func_addr_shift)) {}

uint16_t functions::AD5452::float_to_raw(float scale) {
  // Scaling between +-2 and +-20 coefficient happens outside!
  if (scale <= -2.0f)
    return 0;
  if (scale >= +2.0f)
    return 4095 << 2;
  return static_cast<uint16_t>(scale * 1024.0f + RAW_ZERO) << 2;
}

float functions::AD5452::raw_to_float(uint16_t raw) {
  return (static_cast<float>(raw) - static_cast<float>(RAW_ZERO << 2)) / 4095;
}

void functions::AD5452::set_scale(uint16_t scale_raw) const {
  begin_communication();
  // AD5452 expects at least 13ns delay between chip select and data
  delayNanoseconds(15);
  bus::spi.transfer16(scale_raw);
  end_communication();
}

void functions::AD5452::set_scale(float scale) const {
  // Scaling between +-1 and +-10 coefficient happens outside!
  return set_scale(float_to_raw(scale));
}
