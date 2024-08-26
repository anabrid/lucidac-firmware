// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "AD8402.h"

const SPISettings functions::AD8402::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE2};

functions::AD8402::AD8402(bus::addr_t address, unsigned int base_resistance)
    : functions::DataFunction(address, DEFAULT_SPI_SETTINGS), _base_resistance(base_resistance) {}

void functions::AD8402::write_ch1_resistance(float percent) {
  begin_communication();
  get_raw_spi().transfer16((0b00 << 8 | static_cast<uint8_t>(round(percent * 255))) << 6);
  end_communication();
}

void functions::AD8402::write_ch2_resistance(float percent) {
  begin_communication();
  get_raw_spi().transfer16((0b01 << 8 | static_cast<uint8_t>(round(percent * 255))) << 6);
  end_communication();
}
