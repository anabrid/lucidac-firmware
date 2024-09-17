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

LUCIDACFrontPanel *front;

void test_init() {
  front = entities::detect<LUCIDACFrontPanel>(bus::address_from_tuple(2, 0));
  if (!front)
    TEST_FAIL_MESSAGE("Front plane could not be detected!"); // Avoid nullptr deref

  TEST_ASSERT(front->init());
  TEST_ASSERT(front->signal_generator.is_installed());

  TEST_ASSERT(front->signal_generator.set_amplitude(0.25f));
  TEST_ASSERT(front->signal_generator.set_square_voltage_levels(-1.0f, 1.0f));
  TEST_ASSERT(front->signal_generator.set_offset(0.0f));

  TEST_ASSERT(front->signal_generator.set_dac_out0(1.0f));
  TEST_ASSERT(front->signal_generator.set_dac_out1(-0.5f));

  front->signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE_AND_SQUARE);
  front->signal_generator.set_frequency(1000.0f);

  front->signal_generator.awake();

  TEST_ASSERT_EQUAL(true, front->write_to_hardware());
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

void loop() {
  TEST_ASSERT(
      front->signal_generator.set_amplitude(0.5f * sin((float)millis()) + 0.5f)); // DIY amplitude modulation
  TEST_ASSERT_EQUAL(true, front->write_to_hardware());
}
