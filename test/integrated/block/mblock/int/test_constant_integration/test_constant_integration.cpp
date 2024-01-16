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


void test_constant_integration() {
  auto *intblock = (MIntBlock *)(luci.m1block);
  uint8_t coeff_idx = 0;
  uint8_t adc_channel = 7;

  for(uint8_t i0=0; i0<8; i0++) {
    float factor = 0.5f;
    float ic_value = 0.f;
    unsigned int op_time_us = 100;
    float expected_value = ic_value + factor * static_cast<float>(op_time_us) / 100.0f;

    char buffer[128];
    sprintf(buffer, "Int%d, C_idx=%d, Factor=%f, time=%d us, ACD%d", i0, coeff_idx, factor, op_time_us, adc_channel);
    TEST_MESSAGE(buffer);

    // Reset
    luci.reset(true);
    delayMicroseconds(500);

    // Enable REF signals on U-Block
    TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
    luci.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, coeff_idx, factor, MBlock::M1_INPUT(i0));
    luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);

    if (ANABRID_TESTS_USE_ACL_OUT) {
      // Route the result to an ACL output
      luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::IDX_RANGE_TO_ACL_OUT(5));
    }
  
    intblock->set_ic(i0, ic_value);
    luci.write_to_hardware();
    delayMicroseconds(100);
    ManualControl::to_ic();
    delayMicroseconds(50);
    ManualControl::to_op();
    delayMicroseconds(op_time_us);
    ManualControl::to_halt();
    delayMicroseconds(50);

    float adc_value = daq_.sample()[adc_channel];
    TEST_ASSERT_FLOAT_WITHIN(ANABRID_TESTS_TARGET_PRECISION, expected_value, -adc_value);

    // Change factor to negative
    expected_value = ic_value - factor * static_cast<float>(op_time_us) / 100.0f;

    luci.cblock->set_factor(coeff_idx, -factor);
    luci.write_to_hardware();
    delayMicroseconds(100);
    ManualControl::to_ic();
    delayMicroseconds(50);
    ManualControl::to_op();
    delayMicroseconds(op_time_us);
    ManualControl::to_halt();
    delayMicroseconds(50);

    adc_value = daq_.sample()[adc_channel];
    TEST_ASSERT_FLOAT_WITHIN(ANABRID_TESTS_TARGET_PRECISION, expected_value, -adc_value);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_constant_integration);
  UNITY_END();
}

void loop() {}
