// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "bus/bus.h"

using namespace bus;

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
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

void test_address_register() {
  // Address register GPIO6 is [4bit other][6bit FADDR][4bit BADDR][18bit other]
  TEST_ASSERT_EQUAL(0b0000'000000'1111'000000000000000000, BADDR_BITS_MASK);
  TEST_ASSERT_EQUAL(0b0000'111111'0000'000000000000000000, FADDR_BITS_MASK);
  TEST_ASSERT_EQUAL(0b0000'111111'1111'000000000000000000, ADDR_BITS_MASK);

  address_function(idx_to_addr(0,0,0));
  TEST_ASSERT_BITS(ADDR_BITS_MASK, 0b0000'000000'0001'000000000000000000, GPIO6_DR);
  address_function(idx_to_addr(2,4,0));
  TEST_ASSERT_BITS(ADDR_BITS_MASK, 0b0000'000000'1111'000000000000000000, GPIO6_DR);
  address_function(idx_to_addr(2,4,61));
  TEST_ASSERT_BITS(ADDR_BITS_MASK, 0b0000'111101'1111'000000000000000000, GPIO6_DR);

  address_function_only(13);
  TEST_ASSERT_BITS(ADDR_BITS_MASK, 0b0000'001101'1111'000000000000000000, GPIO6_DR);

  address_board_function(17);
  TEST_ASSERT_BITS(ADDR_BITS_MASK, 0b0000'010001'0000'000000000000000000, GPIO6_DR);
}

void test_address_mutations() {
  TEST_ASSERT_EQUAL(0b101010'0110, replace_function_idx(0b000000'0110, 0b101010));
  TEST_ASSERT_EQUAL(0b011010'0000, remove_addr_parts(0b011010'0110, true, false));
  TEST_ASSERT_EQUAL(0b000000'0110, remove_addr_parts(0b011010'0110, false, true));
  TEST_ASSERT_EQUAL(0b000000'0000, remove_addr_parts(0b011010'0110, true, true));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_idx_to_address);
  RUN_TEST(test_address_register);
  RUN_TEST(test_address_mutations);
  UNITY_END();
}

void loop() {}
