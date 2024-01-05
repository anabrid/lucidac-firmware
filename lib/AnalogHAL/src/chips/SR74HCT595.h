// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "local_bus/functions.h"


namespace functions {

/**
 * The SR74HCT595 is an 8-Bit Shift Register with 3-State Output Registers.
 * It is used for instance at the MIntBlock and the CBlock.
 **/
class SR74HCT595 : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;
  static const SPISettings DEFAULT_SPI_SETTINGS_SHIFTED_CLOCK;

  using DataFunction::DataFunction;
  SR74HCT595(bus::addr_t address, bool shift_clock=false);
};

}