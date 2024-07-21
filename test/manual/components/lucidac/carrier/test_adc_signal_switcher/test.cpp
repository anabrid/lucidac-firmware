// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;

LUCIDAC_HAL hal;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_adc_bus_matrix_reset() { hal.f_adc_switcher_matrix_reset.trigger(); }

void test_adc_bus_matrix() {
  TEST_ASSERT(hal.write_adc_bus_mux({0, 1, 2, 3, 4, 5, 6, 7}));
  // TEST_ASSERT(hal.write_adc_bus_mux({8,9,10,11,12,13,14,15}));
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(test_adc_bus_matrix_reset);
  RUN_TEST(test_adc_bus_matrix);
  // RUN_TEST(test_adc_sr_reset);
  UNITY_END();
}

void loop() {
  // Re-run tests or do an action once the button is pressed
  io::block_until_button_press();
  test_adc_bus_matrix_reset();
  delay(500);
}
