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

namespace local_bus {

extern SPIClass& spi;

using addr_t = uint16_t;

// Block address definitions and helpers
constexpr std::array<uint8_t, 4> PINS_BADDR = {14, 15, 40, 41};
constexpr uint8_t PINS_BADDR_BITS_OFFSET = CORE_PIN14_BIT;
constexpr uint32_t BADDR_BITS_MASK = 0xF << PINS_BADDR_BITS_OFFSET;
// Function address definitions and helpers
constexpr std::array<uint8_t, 6> PINS_FADDR = {17, 16, 22, 23, 20, 21};
constexpr uint8_t PINS_FADDR_BITS_OFFSET = CORE_PIN17_BIT;
constexpr uint32_t FADDR_BITS_MASK = 0x3F << PINS_FADDR_BITS_OFFSET;
// Full address definitions and helpers
constexpr uint8_t PINS_ADDR_BITS_OFFSET = CORE_PIN14_BIT;
constexpr uint32_t ADDR_BITS_MASK = BADDR_BITS_MASK | FADDR_BITS_MASK;

void init();

void _change_address_register(uint32_t clear_mask, uint32_t set_mask);

/**
 * Address an index (0-15) on the local bus.
 * @timing ~25ns (full function call and propagation to final destination)
 * @param idx The index of the block (1-15) or the carrier board (0).
 */
void address_block_idx(uint8_t idx);

/**
 * Address a block inside a cluster.
 * @timing see address_block_idx
 * @param cluster_idx The index of the cluster, from 0 to 2.
 * @param block_idx The index of the block (0=I, 1=C, 2=U, 3=M1, 4=M2).
 */
void address_block(uint8_t cluster_idx, uint8_t block_idx);

/**
 * Address a function without changing the current block address.
 * @timing see address_block_idx
 * @param func_idx The index of the function, from 0 to 63.
 */
void address_function_raw(uint8_t func_idx);

/**
 * Address a function of a block of a cluster.
 * @timing see address_block_idx
 * @param cluster_idx The index of the cluster, from 0 to 2.
 * @param block_idx The index of the block, from 0 to 4 (0=I, 1=C, 2=U, 3=M1, 4=M2).
 * @param func_idx The index of the function, from 0 to 63.
 */
void address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx);

void address_function(addr_t address);

void release_address();

} // namespace local_bus
