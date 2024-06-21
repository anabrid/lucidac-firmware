// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "lucidac/front_plane.h"

using namespace platform;
using namespace lucidac;

FrontPlane front;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();

  front.init();

  TEST_ASSERT(front.signal_generator.set_amplitude(2.5f));
  TEST_ASSERT(front.signal_generator.set_square_voltage_levels(-1.0f, 1.0f));
  TEST_ASSERT(front.signal_generator.set_offset(0.0f));

  TEST_ASSERT(front.signal_generator.set_dac_out0(1.0f));
  TEST_ASSERT(front.signal_generator.set_dac_out1(-1.5f));

  front.signal_generator.set_wave_form(functions::AD9834::WaveForm::SINE_AND_SQUARE);
  front.signal_generator.set_frequency(1000.0f);

  front.signal_generator.awake();

  front.signal_generator.write_to_hardware();

  UNITY_END();
}

void loop() {}
