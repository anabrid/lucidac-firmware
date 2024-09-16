// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "SR74HC16X.h"

const SPISettings functions::SR74HC16X::DEFAULT_SPI_SETTINGS{
    4'000'000, MSBFIRST, SPI_MODE3 /* Chip expects MODE0, CLK is inverted on the way */};

uint8_t functions::SR74HC16X::read8() const { return DataFunction::transfer8(0); }

functions::SR74HC16X::SR74HC16X(bus::addr_t address) : DataFunction(address, DEFAULT_SPI_SETTINGS) {}
