// Copyright (c) 2022 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "mode/mode.h"

using namespace mode;

void setUp() {
  // This is called before *each* test.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
}

void tearDown() {
  // This is called after *each* test.
  digitalWriteFast(LED_BUILTIN, LOW);
}

void interrupt_handler() {
  digitalWriteFast(LED_BUILTIN, LOW);
}

void test_simple_delay_jitter() {
  ManualControl::init();

  for (auto _ : {0,1,2,3,4,5,6,7,8,9}) {
    ManualControl::to_ic();
    // The delay is fairly consistent in its length,
    // but there is a not-really-constant offset to the argument passed.
    // Also note that this is without additional workload on the CPU.
    // 100ns = 123 +- 3.5ns
    // 220ns = 249.5 +- 7ns
    // 230ns = 255.7 +- 1.5ns
    // 250ns = 275.8 +- 1.6ns
    // 500ns = 521.2 +- 3ns
    // 2000ns = 2020.3 +- 0.3ns
    // 5000ns = 5021.4 +- 3ns
    delayNanoseconds(5000);
    ManualControl::to_halt();
    delayNanoseconds(50);
  }
  TEST_ASSERT(true);
}

void test_gpio_interrupt() {
  pinMode(21, INPUT);
  pinMode(PIN_MODE_IC, OUTPUT);
  // Test connection
  digitalWriteFast(PIN_MODE_IC, HIGH);
  delayMicroseconds(1);
  TEST_ASSERT_MESSAGE(digitalReadFast(21), "You must connect pin 4 to pin 23.");
  digitalWriteFast(PIN_MODE_IC, LOW);
  delayMicroseconds(1);

  attachInterrupt(21, interrupt_handler, CHANGE);
  delayMicroseconds(5);
  digitalWriteFast(PIN_MODE_IC, HIGH);
  // This should toggle LED low when IC goes HIGH
  // Measured delay is 90-95ns
  // Note that attachInterrupt on Teensy does not seem to have a list of functions to call,
  // like it does on Arduino (which is really slow), but I'm not perfectly sure.
  // Maybe this could be improved upon.

  delayMicroseconds(5);
  digitalWriteFast(LED_BUILTIN, HIGH);
  TEST_ASSERT(true);
}

void setup() {
  UNITY_BEGIN();
  //RUN_TEST(test_simple_delay_jitter);
  RUN_TEST(test_gpio_interrupt);
  UNITY_END();
}

void loop() { delay(500); }
