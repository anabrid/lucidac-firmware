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

#include "bus.h"

#include "io/ioregister.h"
#include "logging.h"

SPIClass &bus::spi = SPI1;

void bus::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);

  pinMode(PIN_ADDR_CS, OUTPUT);
  digitalWriteFast(PIN_ADDR_CS, HIGH);
  pinMode(PIN_ADDR_LATCH, OUTPUT);
  digitalWriteFast(PIN_ADDR_LATCH, LOW);

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
