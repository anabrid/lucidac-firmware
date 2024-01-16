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

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

#include "test_common.h"

using namespace blocks;
using namespace daq;
using namespace lucidac;
using namespace mode;

LUCIDAC luci{};
OneshotDAQ daq_{};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Initialize mode controller (currently separate thing)
  ManualControl::init();

  // Put LUCIDAC start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(luci.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.m1block, "M1-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_multiplication() {
  auto *multblock = (MIntBlock *)(luci.m1block);
  uint8_t adc_channel =  7;

  for(uint8_t i0=0; i0<4; i0++) {
    char buffer[128];
    sprintf(buffer, "Mult%d ADC%d", i0, adc_channel);
    TEST_MESSAGE(buffer);

    // Reset
    luci.reset(true);
    delayMicroseconds(500);

    // Enable REF signals on U-Block
    TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
    luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);

    if (ANABRID_TESTS_USE_ACL_OUT) {
      // Route the result to an ACL output
      luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::IDX_RANGE_TO_ACL_OUT(5));
    }

    // Route Mult inputs
    luci.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, 0, 0, multblock->M1_INPUT(2*i0));
    luci.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, 1, 0, multblock->M1_INPUT(2*i0+1));
    
    luci.write_to_hardware();
    delayMicroseconds(100);

    float factors[] = { -1, -0.5, 0, 0.5, 1};

    for (float a : factors) {
      for (float b : factors) {
        luci.cblock->set_factor(0,a);
        luci.cblock->set_factor(1,b);
        luci.cblock->write_to_hardware();
        delayMicroseconds(100);

        float adc_value = daq_.sample()[adc_channel];

        TEST_ASSERT_FLOAT_WITHIN(ANABRID_TESTS_TARGET_PRECISION, a*b, -adc_value);

        sprintf(buffer, "%f * %f = %f", a, b, -adc_value);
        TEST_MESSAGE(buffer);

      }
    }
  }

}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_multiplication);
  UNITY_END();
}

void loop() {}
