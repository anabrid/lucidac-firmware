// Copyright (c) 2023 anabrid GmbH
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
