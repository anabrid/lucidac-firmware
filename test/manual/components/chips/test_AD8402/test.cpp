// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "unity.h"
#include <Arduino.h>

#include "chips/AD8402.h"

functions::AD8402 chip(bus::idx_to_addr(0, bus::M1_BLOCK_IDX, 6), 10);

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_basics() { chip.write_ch0_resistance(42);
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(test_basics);
  UNITY_END();
}

void loop() {}
