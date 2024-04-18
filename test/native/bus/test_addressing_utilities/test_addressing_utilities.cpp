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
  TEST_ASSERT_EQUAL(0b000000'0001, idx_to_addr(0, 0, 0));
  TEST_ASSERT_EQUAL(0b000000'0010, idx_to_addr(0, 1, 0));
  TEST_ASSERT_EQUAL(0b000000'0110, idx_to_addr(1, 0, 0));
  TEST_ASSERT_EQUAL(0b000000'0111, idx_to_addr(1, 1, 0));
  TEST_ASSERT_EQUAL(0b111101'0111, idx_to_addr(1, 1, 61));
  TEST_ASSERT_EQUAL(0b000000'1111, idx_to_addr(2, 4, 0));
  TEST_ASSERT_EQUAL(0b000111'1111, idx_to_addr(2, 4, 7));
  // Too high addresses get coerced to zero
  TEST_ASSERT_EQUAL(0b000000'0000, idx_to_addr(2, 5, 0));
  TEST_ASSERT_EQUAL(0b000000'0000, idx_to_addr(3, 0, 0));
  // Way too high addresses probably result in an overflow, but it's useless to test.
}

void test_address_mutations() {
  TEST_ASSERT_EQUAL(0b101010'0110, replace_function_idx(0b000000'0110, 0b101010));
  TEST_ASSERT_EQUAL(0b011010'0000, remove_addr_parts(0b011010'0110, true, false));
  TEST_ASSERT_EQUAL(0b000000'0110, remove_addr_parts(0b011010'0110, false, true));
  TEST_ASSERT_EQUAL(0b000000'0000, remove_addr_parts(0b011010'0110, true, true));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_idx_to_address);
  RUN_TEST(test_address_mutations);
  UNITY_END();
}