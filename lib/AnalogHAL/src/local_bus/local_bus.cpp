// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "local_bus/local_bus.h"

#include "io/ioregister.h"
#include "utils/logging.h"

SPIClass &bus::spi = SPI1;

bus::addr_t bus::idx_to_addr(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  // Address 0 is the carrier-board, blocks start at 1
  // Address is [6bit FADDR][4bit BADDR]
  return (static_cast<addr_t>(func_idx & 0x3F) << 4) + ((cluster_idx * 5 + block_idx + 1) & 0xF);
}

bus::addr_t bus::increase_function_idx(bus::addr_t address, uint8_t delta_idx) {
  return address + (static_cast<addr_t>(delta_idx & 0x3F) << 4);
}

bus::addr_t bus::replace_function_idx(bus::addr_t address, uint8_t func_idx) {
  return (address & ~0x3F0) | (static_cast<addr_t>(func_idx & 0x3F) << 4);
}

bus::addr_t bus::remove_addr_parts(bus::addr_t address, bool block, bool func) {
  if (block)
    address &= ~0xF;
  if (func)
    address &= ~0x3F0;
  return address;
}

bus::addr_t bus::board_function_to_addr(uint8_t func_idx) { return func_idx << 4; }

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
  uint32_t new_bits = address << PINS_ADDR_BIT_SHIFT;
  _change_address_register(ADDR_BITS_MASK, new_bits);
}

void bus::release_address() {
  // TODO: This is currently arbitrary, should be handled as described in
  //       https://lab.analogparadigm.com/lucidac/hardware/module-holder/-/issues/6
  address_function(0, 4, 63);
}

void bus::address_board_function(uint8_t func_idx) { address_function(board_function_to_addr(func_idx)); }
