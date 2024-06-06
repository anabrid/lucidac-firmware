// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "bus/bus.h"
#include "chips/TMP127Q1.h"
#include "io/io.h"

using namespace functions;

// New REV1 addresses
// This one is on C-Block
// TMP127Q1 chip{bus::address_from_tuple(9, 33)};
// This one is on carrier board
TMP127Q1 chip{bus::address_from_tuple(5, 1)};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_raw_conversion() {
  // Assert that right-shift of signed values is an arithmetic shift,
  // which apparently is compiler and processor specific.
  TEST_ASSERT_EQUAL(-800, TMP127Q1::raw_to_signed_raw(0b1111'0011'1000'0011));
}

void test_chip_function() { TEST_ASSERT_FLOAT_WITHIN(10, 30, chip.read_temperature()); }

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(test_raw_conversion);
  RUN_TEST(test_chip_function);
  UNITY_END();
}

void loop() {
  // Do an action once the button is pressed
  while (!io::get_button()) {
  }
  test_chip_function();
  delay(500);
}