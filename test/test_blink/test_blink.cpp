// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

void setUp() {
  // set stuff up here
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
}

void tearDown() {
  // clean stuff up here
}

void test_test() {
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_test);
  UNITY_END();
}

void loop() {
  // blink here
  digitalToggleFast(LED_BUILTIN);
  delay(333);
}