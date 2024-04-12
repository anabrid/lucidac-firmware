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

#pragma once

#include <array>
#include <cstdint>

#include <SPI.h>
#include <core_pins.h>

namespace bus {

extern SPIClass &spi;

// MSBFIRST [16bit] = ADDR_[xx543210] + MADDR_[xxx43210]
using addr_t = uint16_t;

constexpr uint8_t PIN_ADDR_CS = 25;
constexpr uint8_t PIN_ADDR_LATCH = 24;
constexpr uint8_t PIN_ADDR_RESET = 28;

constexpr uint8_t BADDR_MASK = 0x0F;
constexpr uint8_t FADDR_MASK = 0xF0;
constexpr uint8_t FADDR_SHIFT = 8;

// Some BADDRs
constexpr uint8_t CTRL_BLOCK_BADDR = 1;
constexpr uint8_t BACKPLANE_BADDR = 2;
constexpr uint8_t FRONTPLANE_BADDR = 3;
constexpr uint8_t T_BLOCK_BADDR = 4;
constexpr uint8_t CARRIER_BADDR = 5;

constexpr uint8_t BLOCK_BADDR(uint8_t cluster_idx, uint8_t rel_block_idx) {
  return (cluster_idx + 1) * 8 + rel_block_idx;
}

constexpr uint8_t C_BLOCK_IDX = 1;
constexpr uint8_t I_BLOCK_IDX = 2;
constexpr uint8_t M1_BLOCK_IDX = 4;
constexpr uint8_t M2_BLOCK_IDX = 5;
constexpr uint8_t SAH_BLOCK_IDX = 3;
constexpr uint8_t U_BLOCK_IDX = 0;

constexpr uint8_t C_BLOCK_BADDR(uint8_t cluster_idx) { return BLOCK_BADDR(cluster_idx, C_BLOCK_IDX); };

constexpr uint8_t I_BLOCK_BADDR(uint8_t cluster_idx) { return BLOCK_BADDR(cluster_idx, I_BLOCK_IDX); };

constexpr uint8_t M1_BLOCK_BADDR(uint8_t cluster_idx) { return BLOCK_BADDR(cluster_idx, M1_BLOCK_IDX); };

constexpr uint8_t M2_BLOCK_BADDR(uint8_t cluster_idx) { return BLOCK_BADDR(cluster_idx, M2_BLOCK_IDX); };

constexpr uint8_t SAH_BLOCK_BADDR(uint8_t cluster_idx) { return BLOCK_BADDR(cluster_idx, SAH_BLOCK_IDX); };

constexpr uint8_t U_BLOCK_BADDR(uint8_t cluster_idx) { return BLOCK_BADDR(cluster_idx, U_BLOCK_IDX); };

// Common function indexes
constexpr uint8_t METADATA_FUNC_IDX = 0;

constexpr addr_t idx_to_addr(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  return (static_cast<addr_t>(func_idx & FADDR_MASK) << FADDR_SHIFT) + (BLOCK_BADDR(cluster_idx, block_idx) & BADDR_MASK);
}

constexpr addr_t increase_function_idx(addr_t address, uint8_t delta_idx) {
  return address + (static_cast<addr_t>(delta_idx & FADDR_MASK) << FADDR_SHIFT);
}

constexpr addr_t replace_function_idx(addr_t address, uint8_t func_idx) {
  return (address & ~FADDR_MASK) | (static_cast<addr_t>(func_idx & FADDR_MASK) << FADDR_SHIFT);
}

constexpr addr_t remove_addr_parts(addr_t address, bool block, bool func) {
  if (block)
    address &= ~BADDR_MASK;
  if (func)
    address &= ~FADDR_MASK;
  return address;
}

constexpr addr_t address_from_tuple(uint8_t maddr, uint8_t faddr) {
  return (static_cast<uint16_t>(faddr) << FADDR_SHIFT) + maddr;
}

constexpr addr_t NULL_ADDRESS = 0;

void init();

/**
 * Address a function of a block of a cluster.
 * @timing see address_block_idx
 * @param cluster_idx The index of the cluster, from 0 to 2.
 * @param block_idx The index of the block, from 0 to 4 (0=I, 1=C, 2=U, 3=M1, 4=M2).
 * @param func_idx The index of the function, from 0 to 63.
 */
void address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx);

void address_function(uint8_t maddr, uint8_t faddr);

void address_function(addr_t address);

void activate_address();

void deactivate_address();

} // namespace bus
