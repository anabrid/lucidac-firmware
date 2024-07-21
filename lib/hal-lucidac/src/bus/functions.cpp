// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "bus/functions.h"

functions::Function::Function(const bus::addr_t address) : address(address) {}

void functions::TriggerFunction::trigger() const {
  bus::address_function(address);
  bus::activate_address();
  delayNanoseconds(2 * 42);
  bus::deactivate_address();
}

functions::DataFunction::DataFunction(bus::addr_t address, const SPISettings &spiSettings)
    : Function(address), spi_settings(spiSettings) {}

void functions::DataFunction::begin_communication() const {
  bus::address_function(address);
  bus::spi.beginTransaction(spi_settings);
  bus::activate_address();
}

void functions::DataFunction::end_communication() const {
  bus::deactivate_address();
  bus::spi.endTransaction();
}

void functions::DataFunction::transfer(const void *mosi_buf, void *miso_buf, size_t count) const {
  begin_communication();
  bus::spi.transfer(mosi_buf, miso_buf, count);
  end_communication();
}

uint8_t functions::DataFunction::transfer8(uint8_t data_in) const {
  begin_communication();
  auto ret = bus::spi.transfer(data_in);
  end_communication();
  return ret;
}

uint16_t functions::DataFunction::transfer16(uint16_t data_in) const {
  begin_communication();
  auto ret = bus::spi.transfer16(data_in);
  end_communication();
  return ret;
}

uint32_t functions::DataFunction::transfer32(uint32_t data_in) const {
  begin_communication();
  auto ret = bus::spi.transfer32(data_in);
  end_communication();
  return ret;
}

SPIClass &functions::DataFunction::get_raw_spi() { return bus::spi; }
