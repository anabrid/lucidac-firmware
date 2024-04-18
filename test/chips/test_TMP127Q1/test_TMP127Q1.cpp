// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "bus/bus.h"
#include "chips/TMP127Q1.h"

using namespace functions;

TMP127Q1 chip{bus::idx_to_addr(0, bus::C_BLOCK_IDX, 33)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_raw_conversion() {
  // Assert that right-shift of signed values is an arithmetic shift,
  // which apparently is compiler and processor specific.
  TEST_ASSERT_EQUAL(-800, TMP127Q1::raw_to_signed_raw(0b1111'0011'1000'0011));
}

void test_chip_function() {
  TEST_ASSERT_FLOAT_WITHIN(10, 30, chip.read_temperature());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_raw_conversion);
  RUN_TEST(test_chip_function);
  UNITY_END();
}

void loop() {}