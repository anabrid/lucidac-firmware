// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "chips/DAC60508.h"

const SPISettings functions::DAC60508::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE2};

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

#ifdef ANABRID_PEDANTIC
  return data == read_register(address);
#endif
  return true;
}

functions::DAC60508::DAC60508(bus::addr_t address) : functions::DataFunction(address, DEFAULT_SPI_SETTINGS) {}

uint16_t functions::DAC60508::float_to_raw(float value) {
  // This function assumes a 2.5V reference.
  if (value <= 0.0f)
    return RAW_ZERO;
  if (value >= 2.5f)
    return RAW_TWO_FIVE;
  return static_cast<uint16_t>(value / 2.5f * static_cast<float>(0x0FFF)) << 4;
}

float functions::DAC60508::raw_to_float(uint16_t raw) {
  return static_cast<float>(raw >> 4) / static_cast<float>(0x0FFF) * 2.5f;
}

bool functions::DAC60508::set_channel_raw(uint8_t idx, uint16_t value) const {
  return write_register(REG_DAC(idx), value);
}

bool functions::DAC60508::set_channel(uint8_t idx, float value) const {
  // This function assumes a 2.5V reference, otherwise the output voltage does not equal value.
  return write_register(REG_DAC(idx), float_to_raw(value));
}

bool functions::DAC60508::init() const {
  // It's unclear whether these settings are correct for all our purposes,
  // use set_external_reference and set_double_gain to change them after initialization.
  // Compare with https://www.ti.com/lit/ds/symlink/dac60508.pdf for details.
  // REG_CONFIG = 0 sets internal reference, sane ALARM, SDO, no power-down
  // REG_GAIN = 0 sets no ref voltage divisor and gain=1 for all channels
  return write_register(functions::DAC60508::REG_CONFIG, 0b0000'0000'0000'0000) and
         write_register(functions::DAC60508::REG_GAIN, 0b0000'0000'0000'0000);
}

bool functions::DAC60508::set_external_reference(bool set) const {
  auto config = read_register(REG_CONFIG);
  if (set)
    config |= 0b00'00000'1'00000000;
  else
    config &= ~0b00'00000'1'00000000;
  return write_register(REG_CONFIG, config);
}

bool functions::DAC60508::set_double_gain(uint8_t idx, bool set) const {
  if (idx >= 8)
    return false;
  // Set GAIN for one channel
  auto gain = read_register(REG_GAIN);
  uint16_t mask = 1 << idx;
  if (set)
    gain |= mask;
  else
    gain &= ~mask;
  return write_register(REG_GAIN, gain);
}

bool functions::DAC60508::set_double_gain(bool set) const {
  // Set GAIN for all channels
  auto gain = read_register(REG_GAIN);
  uint16_t mask = 0x00FF;
  if (set)
    gain |= mask;
  else
    gain &= ~mask;
  return write_register(REG_GAIN, gain);
}
