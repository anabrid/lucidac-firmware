// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "chips/SR74HCT595.h"

const SPISettings functions::SR74HCT595::DEFAULT_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE3 /* Chip expects MODE0, CLK is inverted on the way */};

const SPISettings functions::SR74HCT595::DEFAULT_SPI_SETTINGS_SHIFTED_CLOCK{
    4'000'000, MSBFIRST,
    SPI_MODE2 /* Chip expects MODE0, CLK is inverted on the way, but MOSI is not, thus CLK must be shifted */};

functions::SR74HCT595::SR74HCT595(bus::addr_t address, bool shift_clock)
    : functions::DataFunction(address,
                              shift_clock ? DEFAULT_SPI_SETTINGS_SHIFTED_CLOCK : DEFAULT_SPI_SETTINGS) {}

bool functions::SR74HCT595::transfer8(uint8_t data_in, uint8_t *data_out) const {
#ifndef ANABRID_PEDANTIC
  uint8_t ret = DataFunction::transfer8(data_in);
  if (data_out)
    *data_out = ret;
  return true;

#else
  DataFunction::transfer8(data_in);
  uint8_t written_data = DataFunction::transfer8(data_in);

  if (data_out)
    *data_out = written_data;

  return written_data == data_in;
#endif
}

bool functions::SR74HCT595::transfer16(uint16_t data_in, uint16_t *data_out) const {
#ifndef ANABRID_PEDANTIC
  uint16_t ret = DataFunction::transfer16(data_in);
  if (data_out)
    *data_out = ret;
  return true;

#else
  DataFunction::transfer16(data_in);
  uint16_t written_data = DataFunction::transfer16(data_in);

  if (data_out)
    *data_out = written_data;

  return written_data == data_in;
#endif
}

bool functions::SR74HCT595::transfer32(uint32_t data_in, uint32_t *data_out) const {
#ifndef ANABRID_PEDANTIC
  uint32_t ret = DataFunction::transfer32(data_in);
  if (data_out)
    *data_out = ret;
  return true;

#else
  DataFunction::transfer32(data_in);
  uint32_t written_data = DataFunction::transfer32(data_in);

  if (data_out)
    *data_out = written_data;

  return written_data == data_in;
#endif
}