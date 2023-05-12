// Copyright (c) 2023 anabrid GmbH
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
#include <array>
#include <unity.h>

#include "local_bus.h"

namespace daq {

constexpr uint8_t NUM_CHANNELS = 8;
constexpr uint8_t PIN_CNVST = 7;
constexpr uint8_t PIN_CLK = 6;
constexpr std::array<uint8_t, NUM_CHANNELS> PINS_MISO = {34, 35, 36, 37, 11, 10, 9, 8};

class OneshotDAQ {
public:
  void init() {
    pinMode(PIN_CNVST, OUTPUT);
    digitalWriteFast(PIN_CNVST, LOW);
    pinMode(PIN_CLK, OUTPUT);
    digitalWriteFast(PIN_CLK, LOW);

    for (auto pin : PINS_MISO) {
      // Pull-up is on hardware
      pinMode(pin, INPUT);
    }
  }

  std::array<uint16_t, NUM_CHANNELS> sample() {
    // Trigger CNVST
    digitalWriteFast(PIN_CNVST, HIGH);
    delayNanoseconds(1500);
    digitalWriteFast(PIN_CNVST, LOW);

    delayNanoseconds(500);

    for (auto clk_i = 0; clk_i < 16; clk_i++) {
      digitalWriteFast(PIN_CLK, HIGH);
      delayNanoseconds(200);
      digitalWriteFast(PIN_CLK, LOW);
      delayNanoseconds(200);
    }

    return {0};
  }
};

} // namespace daq

using namespace daq;
OneshotDAQ DAQ{};

void setUp() {
  // set stuff up here
  bus::init();
  DAQ.init();
}

void tearDown() {
  // clean stuff up here
}

void test_true() {
  TEST_ASSERT(true);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_true);
  UNITY_END();
}

void loop() {
  delayMicroseconds(100);
  DAQ.sample();
}