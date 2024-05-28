// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

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

void test_adc_switcher_matrix_reset() { carrier_board.f_adc_switcher_matrix_reset.trigger(); }

void test_adc_prg() {
  carrier_board.f_adc_switcher_prg.transfer8(functions::ICommandRegisterFunction::chip_cmd_word(3, 5));
  carrier_board.f_adc_switcher_sync.trigger();
  TEST_ASSERT(true);
}

void test_adc_sr_reset() {
  carrier_board.f_adc_switcher_sr_reset.trigger();
  TEST_ASSERT(true);
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();
  // RUN_TEST(test_init);
  RUN_TEST(test_adc_switcher_matrix_reset);
  RUN_TEST(test_adc_prg);
  // RUN_TEST(test_adc_sr_reset);
  UNITY_END();
}

void loop() {
  // Re-run tests or do an action once the button is pressed
  while (digitalReadFast(29)) {
  }
  test_adc_switcher_matrix_reset();
  delay(500);
}
