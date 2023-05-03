// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include "local_bus.h"

#include "ioregister.h"

SPIClass& local_bus::spi = SPI1;

local_bus::addr_t local_bus::idx_to_addr(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  // Address 0 is the carrier-board, blocks start at 1
  // Address is [6bit FADDR][4bit BADDR]
  return  ((func_idx & 0x3F) << 4) + ((cluster_idx * 5 + block_idx + 1) & 0xF);
}

local_bus::addr_t local_bus::board_function_to_addr(uint8_t func_idx) {
  return func_idx << 4;
}

void local_bus::init() {
  for (const auto pin : PINS_BADDR) {
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, LOW);
  }
  for (const auto pin : PINS_FADDR) {
    pinMode(pin, OUTPUT);
    digitalWriteFast(pin, LOW);
  }
  local_bus::release_address();
  local_bus::spi.begin();
  local_bus::spi.setMISO(39);
}

void local_bus::_change_address_register(uint32_t clear_mask, uint32_t set_mask) {
  io::change_register(GPIO6_DR, clear_mask, set_mask);
}

void local_bus::address_block(uint8_t cluster_idx, uint8_t block_idx) {
  address_function(cluster_idx, block_idx, 0);
}

void local_bus::address_function_only(uint8_t func_idx) {
  uint32_t mask = (func_idx & 0x3F) << PINS_FADDR_BIT_SHIFT;
  _change_address_register(FADDR_BITS_MASK, mask);
}

void local_bus::address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  address_function(idx_to_addr(cluster_idx, block_idx, func_idx));
}

void local_bus::address_function(local_bus::addr_t address) {
  uint32_t new_bits = address << PINS_ADDR_BIT_SHIFT;
  _change_address_register(ADDR_BITS_MASK, new_bits);
}

void local_bus::release_address() {
  // TODO: This is currently arbitrary, should be handled as described in
  //       https://lab.analogparadigm.com/lucidac/hardware/module-holder/-/issues/6
  address_function(0, 4, 63);
}

void local_bus::address_board_function(uint8_t func_idx) {
  address_function(board_function_to_addr(func_idx));
}

void local_bus::Function::begin_communication() const {
  spi.beginTransaction(spi_settings);
  local_bus::address_function(address);
}

void local_bus::Function::end_communication() const {
  local_bus::release_address();
  spi.endTransaction();
}
