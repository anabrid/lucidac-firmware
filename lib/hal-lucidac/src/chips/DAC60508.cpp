// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "DAC60508.h"

const SPISettings functions::DAC60508::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE1};

uint16_t functions::DAC60508::read_register(uint8_t address) const {
  begin_communication();
  get_raw_spi().transfer((address & 0b0'000'1111) | 0b1'000'0000);
  get_raw_spi().transfer16(0);
  end_communication();
  begin_communication();
  get_raw_spi().transfer(0);
  auto data = get_raw_spi().transfer16(0);
  end_communication();
  return data;
}

bool functions::DAC60508::write_register(uint8_t address, uint16_t data) const {
  begin_communication();
  get_raw_spi().transfer(address & 0b0'000'1111);
  get_raw_spi().transfer16(data);
  end_communication();
  // One could possibly read the register back to check or use CRC
  return true;
}

functions::DAC60508::DAC60508(bus::addr_t address) : functions::DataFunction(address, DEFAULT_SPI_SETTINGS) {}

uint16_t functions::DAC60508::float_to_raw(float value) {
  // TODO: Potentially, each channel must be calibrated.
  if (value <= -1.0f)
    return RAW_MINUS_ONE;
  if (value >= +1.0f)
    return RAW_PLUS_ONE;
  return static_cast<uint16_t>((value + 1.0f) / 2.0f * static_cast<float>(RAW_PLUS_ONE - RAW_MINUS_ONE));
}

float functions::DAC60508::raw_to_float(uint16_t raw) {
  return (static_cast<float>(raw) - static_cast<float>(RAW_MINUS_ONE)) /
             static_cast<float>(RAW_PLUS_ONE - RAW_MINUS_ONE) * 2.0f -
         1.0f;
}

bool functions::DAC60508::set_channel(uint8_t idx, uint16_t value) const {
  write_register(REG_DAC(idx), value);
  // One could possibly read the register back to check or use CRC
  return true;
}

void functions::DAC60508::init() const {
  // TODO: Change back to external reference when working again
  write_register(functions::DAC60508::REG_CONFIG, 0b0000'0000'0000'0000);
  write_register(functions::DAC60508::REG_GAIN, 0b0000'0000'1111'1111);
}
