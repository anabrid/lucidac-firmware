// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "lucidac/front_panel.h"

using namespace platform;

LUCIDACFrontPanel front;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void all_on() {
  front.leds.set_all(0xff);
  TEST_ASSERT(front.leds.write_to_hardware());

  delay(3000);

  front.leds.reset();
  TEST_ASSERT(front.leds.write_to_hardware());
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  Serial.begin(9600);

  UNITY_BEGIN();

  RUN_TEST(all_on);

  UNITY_END();
}

void loop() {
  for (int i = 0; i < 8; i++) {
    front.leds.set(i, true);
    front.leds.write_to_hardware();
    delay(200);
    front.leds.set(i, false);
    front.leds.write_to_hardware();
  }
  for (int i = 7; i >= 0; i--) {
    front.leds.set(i, true);
    front.leds.write_to_hardware();
    delay(200);
    front.leds.set(i, false);
    front.leds.write_to_hardware();
  }
}
