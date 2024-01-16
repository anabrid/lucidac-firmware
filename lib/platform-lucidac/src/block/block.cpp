// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/block.h"
#include "bus/functions.h"
#include "bus/bus.h"

void blocks::CScaleSwitchFunction::write_to_hardware() const {
  begin_communication();
  bus::spi.transfer32(data);
  end_communication();
}

blocks::CScaleSwitchFunction::CScaleSwitchFunction(bus::addr_t address)
    : _old_DataFunction(address, SPISettings(4'000'000, MSBFIRST,
                                        SPI_MODE3 /* Chip expects MODE0, CLK is inverted on the way */)),
      data(0) {}

blocks::CCoeffFunction::CCoeffFunction(bus::addr_t base_address, uint8_t coeff_idx)
    : _old_DataFunction(bus::increase_function_idx(base_address, coeff_idx),
                   SPISettings(4'000'000, MSBFIRST, SPI_MODE1)),
      data{0} {}

void blocks::CCoeffFunction::write_to_hardware() const {
  begin_communication();
  // AD5452 expects at least 13ns delay between chip select and data
  delayNanoseconds(15);
  bus::spi.transfer16(data);
  end_communication();
}
