// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "bus/functions.h"

functions::Function::Function(const bus::addr_t address) : address(address) {}

void functions::Function::set_address() const { bus::address_function(address); }

void functions::Function::release_address() const { bus::release_address(); }

void functions::_old_DataFunction::begin_communication() const {
  bus::spi.beginTransaction(spi_settings);
  set_address();
}

void functions::_old_DataFunction::end_communication() const {
  release_address();
  bus::spi.endTransaction();
}

functions::_old_DataFunction::_old_DataFunction(const unsigned short address, const SPISettings &spiSettings)
    : Function(address), spi_settings(spiSettings) {}

void functions::TriggerFunction::trigger() const {
  set_address();
  delayNanoseconds(2 * 42);
  release_address();
}

functions::DataFunction::DataFunction(bus::addr_t address, const SPISettings &spiSettings)
    : Function(address), spi_settings(spiSettings) {}

void functions::DataFunction::begin_communication() const {
  bus::spi.beginTransaction(spi_settings);
  set_address();
}

void functions::DataFunction::end_communication() const {
  release_address();
  bus::spi.endTransaction();
}

uint8_t functions::DataFunction::transfer(uint8_t data) const {
  begin_communication();
  auto ret = bus::spi.transfer(data);
  end_communication();
  return ret;
}

void functions::DataFunction::transfer(const void *mosi_buf, void *miso_buf, size_t count) {
  begin_communication();
  bus::spi.transfer(mosi_buf, miso_buf, count);
  end_communication();
}

uint16_t functions::DataFunction::transfer16(uint16_t data) const {
  begin_communication();
  auto ret = bus::spi.transfer16(data);
  end_communication();
  return ret;
}

uint32_t functions::DataFunction::transfer32(uint32_t data) const {
  begin_communication();
  auto ret = bus::spi.transfer32(data);
  end_communication();
  return ret;
}

SPIClass &functions::DataFunction::get_raw_spi() { return bus::spi; }
