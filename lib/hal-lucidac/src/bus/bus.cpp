// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "bus/bus.h"

#include "utils/logging.h"

SPIClass &bus::spi = BUS_SPI_INTERFACE;

FLASHMEM void bus::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);

  pinMode(PIN_ADDR_CS, OUTPUT);
  digitalWriteFast(PIN_ADDR_CS, HIGH);
  pinMode(PIN_ADDR_LATCH, OUTPUT);
  digitalWriteFast(PIN_ADDR_LATCH, LOW);
  pinMode(PIN_ADDR_RESET, OUTPUT);
  digitalWriteFast(PIN_ADDR_RESET, HIGH);

  bus::spi.begin();
  bus::spi.setMISO(39);
  bus::deactivate_address();
}

void bus::address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  address_function(idx_to_addr(cluster_idx, block_idx, func_idx));
}

void bus::address_function(uint8_t maddr, uint8_t faddr) {
  address_function(address_from_tuple(maddr, faddr));
}

void bus::address_function(bus::addr_t address) {
  bus::spi.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE2));
  delayNanoseconds(200);
  digitalWriteFast(PIN_ADDR_CS, LOW);
  delayNanoseconds(200);
  bus::spi.transfer16(address);
  delayNanoseconds(200);
  digitalWriteFast(PIN_ADDR_CS, HIGH);
  delayNanoseconds(200);
  bus::spi.endTransaction();
}

void bus::deactivate_address() {
  digitalWriteFast(PIN_ADDR_RESET, LOW);
  delayNanoseconds(200);
  activate_address();
  delayNanoseconds(200);
  digitalWriteFast(PIN_ADDR_RESET, HIGH);
}

void bus::activate_address() {
  digitalWriteFast(PIN_ADDR_LATCH, HIGH);
  delayNanoseconds(200);
  digitalWriteFast(PIN_ADDR_LATCH, LOW);
}
