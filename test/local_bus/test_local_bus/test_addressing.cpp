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

#include <Arduino.h>
#include <unity.h>

#include "local_bus.h"

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

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_idx_to_address);
  RUN_TEST(test_address_register);
  UNITY_END();
}

void loop() {}
