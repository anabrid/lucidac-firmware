// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <unity.h>

#include "bus/bus.h"

using namespace bus;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_idx_to_address() {
  // MSBFIRST [16bit] = ADDR_[xx543210] + MADDR_[xxx43210]
  TEST_ASSERT_EQUAL(0b00000000'00001000, idx_to_addr(0, 0, 0));
  TEST_ASSERT_EQUAL(0b00000000'00001001, idx_to_addr(0, 1, 0));
  TEST_ASSERT_EQUAL(0b00000000'00010000, idx_to_addr(1, 0, 0));
  TEST_ASSERT_EQUAL(0b00000000'00010001, idx_to_addr(1, 1, 0));
  TEST_ASSERT_EQUAL(0b00111101'00010001, idx_to_addr(1, 1, 61));
  TEST_ASSERT_EQUAL(0b00000000'00011100, idx_to_addr(2, 4, 0));
  TEST_ASSERT_EQUAL(0b00000111'00011100, idx_to_addr(2, 4, 7));
}

void test_max_addresses() {
  // MSBFIRST [16bit] = ADDR_[xx543210] + MADDR_[xxx43210]
  // We have a maximum of 6 FADDR bits and mask higher bits
  TEST_ASSERT_EQUAL(0b00000000'00001000, idx_to_addr(0, 0, 0));
  TEST_ASSERT_EQUAL(0b00111101'00001000, idx_to_addr(0, 0, 61));
  TEST_ASSERT_EQUAL(0b00111111'00001000, idx_to_addr(0, 0, 63));
  TEST_ASSERT_EQUAL(0b00000000'00001000, idx_to_addr(0, 0, 64));
  TEST_ASSERT_EQUAL(0b00111111'00001000, idx_to_addr(0, 0, 127));
  // We have a maximum of 5 MADDR bits and mask higher bits
  TEST_ASSERT_EQUAL(0b00001101'00001000, idx_to_addr(0, 0, 13));
  TEST_ASSERT_EQUAL(0b00001110'00011000, idx_to_addr(2, 0, 14));
  TEST_ASSERT_EQUAL(0b00001111'00000000, idx_to_addr(3, 0, 15));
  TEST_ASSERT_EQUAL(0b00010000'00000000, address_from_tuple(0, 16));
  TEST_ASSERT_EQUAL(0b00010001'00011111, address_from_tuple(31, 17));
  TEST_ASSERT_EQUAL(0b00010010'00000000, address_from_tuple(32, 18));
}

void test_address_mutations() {
  // MSBFIRST [16bit] = ADDR_[xx543210] + MADDR_[xxx43210]
  TEST_ASSERT_EQUAL(0b00101010'00011000, replace_function_idx(0b00000000'00011000, 0b101010));
  TEST_ASSERT_EQUAL(0b00011010'00000000, remove_addr_parts(0b00011010'00011000, true, false));
  TEST_ASSERT_EQUAL(0b00000000'00011000, remove_addr_parts(0b00011010'00011000, false, true));
  TEST_ASSERT_EQUAL(0b00000000'00000000, remove_addr_parts(0b00011010'00011000, true, true));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_idx_to_address);
  RUN_TEST(test_max_addresses);
  RUN_TEST(test_address_mutations);
  UNITY_END();
}
