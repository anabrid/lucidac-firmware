// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/functions.h"


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

  bool transfer8(uint8_t data_in, uint8_t *data_out = nullptr) const;
  bool transfer16(uint16_t data_in, uint16_t *data_out = nullptr) const;
  bool transfer32(uint32_t data_in, uint32_t *data_out = nullptr) const;
};

}