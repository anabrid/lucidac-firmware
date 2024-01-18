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
#include <unity.h>

#include "carrier/cluster.h"
#include "daq/daq.h"

using namespace lucidac;
using namespace blocks;
using namespace daq;

LUCIDAC luci{};
OneshotDAQ daq_{};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Put LUCIDAC start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(luci.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.iblock, "I-Block not inserted");
  //TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.m2block, "M2-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_function() {
  // Connect -1*REF to U-Block outputs 0 & 1
  TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  TEST_ASSERT(luci.ublock->connect(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, 0));
  TEST_ASSERT(luci.ublock->connect(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, 1));
  // Scale by -0.5 to get +0.5 on both wires
  TEST_ASSERT(luci.cblock->set_factor(0, -0.5));
  TEST_ASSERT(luci.cblock->set_factor(1, -0.5));
  // Send through I-Block to M2-Block (outputs 0-7 go to M2, 8-15 to M1)
  TEST_ASSERT(luci.iblock->connect(0,0));
  TEST_ASSERT(luci.iblock->connect(1,1));
  // In M2-Block, the MUL block should be
  // Resulting in 0.25 as output on BL_OUT.0
  // Connect this to an ADC to measure
  uint8_t adc_channel = 2;
  TEST_ASSERT(luci.ublock->connect(MBlock::M2_OUTPUT(0),adc_channel));
  // Write config to hardware
  luci.write_to_hardware();

  // Measure result
  delay(10);
  auto data = daq_.sample();
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -0.25f, data[adc_channel]);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
