// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "lucidac/front_panel.h"

using namespace platform;

LUCIDACFrontPanel *front;

void init() {
  front = entities::detect<LUCIDACFrontPanel>(bus::address_from_tuple(2, 0));
  if (!front)
    TEST_FAIL_MESSAGE("Front plane could not be detected!"); // Avoid nullptr deref

  TEST_ASSERT(front->init());

  front->leds.reset(); // Init just triggeres this function for the LEDs, as we don't want to assume having a
                       // signal generator installed here
}

void test_all_on() {
  front->leds.set_all(0xff);
  TEST_ASSERT(front->leds.write_to_hardware());

  delay(3000);

  front->leds.reset();
  TEST_ASSERT(front->leds.write_to_hardware());
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_all_on);
  UNITY_END();
}

void loop() {
  for (int i = 0; i < 8; i++) {
    front->leds.set(i, true);
    front->leds.write_to_hardware();
    delay(200);
    front->leds.set(i, false);
    front->leds.write_to_hardware();
  }
  for (int i = 7; i >= 0; i--) {
    front->leds.set(i, true);
    front->leds.write_to_hardware();
    delay(200);
    front->leds.set(i, false);
    front->leds.write_to_hardware();
  }
}
