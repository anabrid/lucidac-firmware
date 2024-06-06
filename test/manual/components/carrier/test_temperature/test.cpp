// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "io/io.h"

#define private public
#define protected public
#include "carrier/carrier.h"

using namespace carrier;

Carrier carrier_board({});

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() { TEST_ASSERT(carrier_board.init()); }

void test_temperature() { TEST_ASSERT_FLOAT_WITHIN(2, 25.0f, carrier_board.f_temperature.read_temperature()); }

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  // RUN_TEST(test_init);
  RUN_TEST(test_temperature);
  UNITY_END();
}

void loop() {
  // Do an action once the button is pressed
  io::block_until_button_press();
  test_temperature();
  delay(500);
}
