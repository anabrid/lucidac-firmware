// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "io/io.h"
#include "mode/mode.h"

using namespace mode;
using namespace io;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_mode_leds() {
  // Default-initialized state
  TEST_MESSAGE("NONE of the LEDs should be on.");
  block_until_button_press_and_release();

  ManualControl::to_ic();
  TEST_MESSAGE("IC should be on, rest should be off.");
  block_until_button_press_and_release();

  ManualControl::to_op();
  TEST_MESSAGE("OP should be on, rest should be off.");
  block_until_button_press_and_release();

  digitalWriteFast(PIN_MODE_EXTHALT, LOW);
  TEST_MESSAGE("OP & (EXT)HALT should be on, rest should be off.");
  block_until_button_press_and_release();

  digitalWriteFast(PIN_MODE_OVERLOAD, LOW);
  TEST_MESSAGE("OP & (EXT)HALT & OL should be on, IC should be off.");
  block_until_button_press_and_release();

  ManualControl::to_ic();
  TEST_MESSAGE("IC & (EXT)HALT & OL should be on, OP should be off.");
  block_until_button_press_and_release();

  digitalWriteFast(PIN_MODE_EXTHALT, HIGH);
  TEST_MESSAGE("IC & OL should be on, rest should be off.");
  block_until_button_press_and_release();

  ManualControl::to_halt();
  TEST_MESSAGE("OL should be on, rest should be off.");
  block_until_button_press_and_release();

  digitalWriteFast(PIN_MODE_OVERLOAD, HIGH);
  TEST_MESSAGE("NONE of the LEDs should be on.");
  block_until_button_press_and_release();
}

void setup() {
  io::init();
  ManualControl::init();
  // For this test case, overwrite EXT_HALT and OVERLOAD signals
  pinMode(PIN_MODE_EXTHALT, OUTPUT);
  pinMode(PIN_MODE_OVERLOAD, OUTPUT);
  digitalWriteFast(PIN_MODE_EXTHALT, HIGH);
  digitalWriteFast(PIN_MODE_OVERLOAD, HIGH);

  UNITY_BEGIN();
  RUN_TEST(test_mode_leds);
  UNITY_END();
}

void loop() {}
