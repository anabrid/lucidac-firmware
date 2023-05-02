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

void local_bus::init() {
  for (const auto pin : PINS_BADDR) {
    pinMode(pin, OUTPUT);
  }
  for (const auto pin : PINS_FADDR) {
    pinMode(pin, OUTPUT);
  }
  local_bus::spi.begin();
  local_bus::spi.setMISO(39);
}

void local_bus::_change_address_register(uint32_t clear_mask, uint32_t set_mask) {
  io::change_register(GPIO6_DR, clear_mask, set_mask);
}

void local_bus::address_block_idx(uint8_t idx) {
  uint32_t mask = (idx & 0xF) << PINS_BADDR_BITS_OFFSET;
  _change_address_register(BADDR_BITS_MASK, mask);
}

void local_bus::address_block(uint8_t cluster_idx, uint8_t block_idx) {
  address_block_idx(cluster_idx * 5 + block_idx);
}

void local_bus::address_function_raw(uint8_t func_idx) {
  uint32_t mask = (func_idx & 0x3F) << PINS_FADDR_BITS_OFFSET;
  _change_address_register(FADDR_BITS_MASK, mask);
}

void local_bus::address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  uint32_t mask = (((cluster_idx * 5 + block_idx) & 0xF) << PINS_BADDR_BITS_OFFSET) |
                  ((func_idx & 0x3F) << PINS_FADDR_BITS_OFFSET);
  _change_address_register(ADDR_BITS_MASK, mask);
}

void local_bus::address_function(local_bus::addr_t address) {
  uint32_t new_bits = address << PINS_FADDR_BITS_OFFSET;
  _change_address_register(ADDR_BITS_MASK, new_bits);
}

void local_bus::release_address() {
  // TODO: This is currently arbitrary, should be handled as described in
  //       https://lab.analogparadigm.com/lucidac/hardware/module-holder/-/issues/6
  address_function(0, 4, 63);
}

void local_bus::Function::start_communication() const {
  spi.beginTransaction(spi_settings);
  local_bus::address_function(address);
}

void local_bus::Function::end_communication() const {
  local_bus::release_address();
  spi.endTransaction();
}
