// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "TMP127Q1.h"

const SPISettings functions::TMP127Q1::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE0};

float functions::TMP127Q1::read_temperature() const {
  auto raw = transfer16(0);
  return raw_to_float(raw);
}

float functions::TMP127Q1::raw_to_float(uint16_t raw) {
  auto signed_raw = raw_to_signed_raw(raw);
  return static_cast<float>(signed_raw) * 0.03125f;
}

int16_t functions::TMP127Q1::raw_to_signed_raw(uint16_t raw) {
  // Convert to signed integer and use arithmetic right-shift, which keeps sign bit.
  // Last two bits are always 1 and need to be shifted out to result in 14bit value.
  auto signed_raw = static_cast<int16_t>(raw);
  return static_cast<int16_t>(signed_raw >> 2);
}

functions::TMP127Q1::TMP127Q1(unsigned short address) : DataFunction(address, DEFAULT_SPI_SETTINGS) {}
