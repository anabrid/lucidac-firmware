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
#include "mode/mode.h"

using namespace platform;

LUCIDACFrontPanel front;

void test_init() {
  TEST_ASSERT(front.init());

  TEST_ASSERT(front.signal_generator.set_amplitude(2.5f));
  TEST_ASSERT(front.signal_generator.set_square_voltage_levels(-2.0f, 2.0f));
  TEST_ASSERT(front.signal_generator.set_offset(0.0f));

  TEST_ASSERT(front.signal_generator.set_dac_out0(1.0f));
  TEST_ASSERT(front.signal_generator.set_dac_out1(-1.5f));

  front.signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE_AND_SQUARE);
  front.signal_generator.set_frequency(1000.0f);

  front.signal_generator.awake();

  TEST_ASSERT_EQUAL(true, front.write_to_hardware());
}

void setup() {
  bus::init();
  mode::ManualControl::init();
  mode::ManualControl::to_op();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();

  RUN_TEST(test_init);

  UNITY_END();
}

void loop() { /*
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
   */
  TEST_ASSERT(front.signal_generator.set_amplitude(2.5f / 2.0f * (sin((float)millis() * 0.25f) + 1.0f)));
  TEST_ASSERT_EQUAL(true, front.write_to_hardware());
}
