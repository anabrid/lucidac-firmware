// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "bus.h"

#include "io/ioregister.h"
#include "logging.h"

SPIClass &bus::spi = BUS_SPI_INTERFACE;

void bus::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  for (const auto pin : PINS_BADDR) {
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, LOW);
  }
  for (const auto pin : PINS_FADDR) {
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, LOW);
  }
  bus::release_address();
  bus::spi.begin();
  bus::spi.setMISO(39);
}

void bus::_change_address_register(uint32_t clear_mask, uint32_t set_mask) {
  io::change_register(GPIO6_DR, clear_mask, set_mask);
}

void bus::address_block(uint8_t cluster_idx, uint8_t block_idx) {
  address_function(cluster_idx, block_idx, 0);
}

void bus::address_function_only(uint8_t func_idx) {
  uint32_t mask = (func_idx & 0x3F) << PINS_FADDR_BIT_SHIFT;
  _change_address_register(FADDR_BITS_MASK, mask);
}

void bus::address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  address_function(idx_to_addr(cluster_idx, block_idx, func_idx));
}

void bus::address_function(bus::addr_t address) {
  _change_address_register(ADDR_BITS_MASK, bus::address_to_register(address));
}

void bus::release_address() {
  address_function(bus::NULL_ADDRESS);
}

void bus::address_board_function(uint8_t func_idx) { address_function(board_function_to_addr(func_idx)); }
