// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>
#include <tuple>

#include <Arduino.h>

// Allow mocking the hardware in native unit tests
// TODO: Instead add SPI1 to ArduinoTeensyFake
#if defined(ARDUINO)
#define BUS_SPI_INTERFACE SPI1;
#include <SPI.h>
#else
#define BUS_SPI_INTERFACE SPI;
#endif

namespace bus {

extern SPIClass &spi;

using addr_t = uint16_t;

// Block address definitions and helpers
constexpr std::array<uint8_t, 4> PINS_BADDR = {14, 15, 40, 41};
constexpr uint8_t PINS_BADDR_BIT_SHIFT = CORE_PIN14_BIT;
constexpr uint32_t BADDR_BITS_MASK = 0xF << PINS_BADDR_BIT_SHIFT;
// Function address definitions and helpers
constexpr std::array<uint8_t, 6> PINS_FADDR = {17, 16, 22, 23, 20, 21};
constexpr uint8_t PINS_FADDR_BIT_SHIFT = CORE_PIN17_BIT;
constexpr uint32_t FADDR_BITS_MASK = 0x3F << PINS_FADDR_BIT_SHIFT;
// Full address definitions and helpers
// Address register GPIO6 is [4bit other][6bit FADDR][4bit BADDR][18bit other]
constexpr uint8_t PINS_ADDR_BIT_SHIFT = PINS_BADDR_BIT_SHIFT;
constexpr uint32_t ADDR_BITS_MASK = BADDR_BITS_MASK | FADDR_BITS_MASK;
// Block indexes
constexpr uint8_t I_BLOCK_IDX = 0;
constexpr uint8_t C_BLOCK_IDX = 1;
constexpr uint8_t U_BLOCK_IDX = 2;
constexpr uint8_t M1_BLOCK_IDX = 3;
constexpr uint8_t M2_BLOCK_IDX = 4;
// Common function indexes
constexpr uint8_t METADATA_FUNC_IDX = 0;

constexpr addr_t idx_to_addr(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx) {
  return (static_cast<addr_t>(func_idx & 0x3F) << 4) + ((cluster_idx * 5 + block_idx + 1) & 0xF);
}

constexpr addr_t increase_function_idx(addr_t address, uint8_t delta_idx) {
  return address + (static_cast<addr_t>(delta_idx & 0x3F) << 4);
}

constexpr addr_t replace_function_idx(addr_t address, uint8_t func_idx) {
  return (address & ~0x3F0) | (static_cast<addr_t>(func_idx & 0x3F) << 4);
}

constexpr addr_t remove_addr_parts(addr_t address, bool block, bool func) {
  if (block)
    address &= ~0xF;
  if (func)
    address &= ~0x3F0;
  return address;
}

constexpr addr_t board_function_to_addr(uint8_t func_idx) { return func_idx << 4; }

// TODO: This is currently arbitrary, should be handled as described in
//       https://lab.analogparadigm.com/lucidac/hardware/module-holder/-/issues/6
constexpr addr_t NULL_ADDRESS = idx_to_addr(0, 4, 63);

void init();

constexpr uint32_t address_to_register(addr_t address) { return address << PINS_ADDR_BIT_SHIFT; }

void _change_address_register(uint32_t clear_mask, uint32_t set_mask);

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
void address_function_only(uint8_t func_idx);

/**
 * Address a function of a block of a cluster.
 * @timing see address_block_idx
 * @param cluster_idx The index of the cluster, from 0 to 2.
 * @param block_idx The index of the block, from 0 to 4 (0=I, 1=C, 2=U, 3=M1, 4=M2).
 * @param func_idx The index of the function, from 0 to 63.
 */
void address_function(uint8_t cluster_idx, uint8_t block_idx, uint8_t func_idx);

void address_function(addr_t address);

void address_board_function(uint8_t func_idx);

void release_address();

} // namespace bus
