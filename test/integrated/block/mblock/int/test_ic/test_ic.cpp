// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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

void test_ic() {
  auto *intblock = (MIntBlock *)(luci.m1block);
  uint8_t adc_channel =  7;

  for(uint8_t i0=0; i0<8; i0++) {
    char buffer[128];
    sprintf(buffer, "Int%d ADC%d", i0, adc_channel);
    TEST_MESSAGE(buffer);

    // Reset 
    luci.reset(true);
    delayMicroseconds(500);

    luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);

    if (ANABRID_TESTS_USE_ACL_OUT) {
      // Route the result to an ACL output
      luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::IDX_RANGE_TO_ACL_OUT(5));
    }

    for (float ic_value = -1; ic_value <= 1; ic_value += 0.001) {
      intblock->set_ic(i0, ic_value);
      luci.write_to_hardware();
      delayMicroseconds(100);

      ManualControl::to_ic();
      delayMicroseconds(50);
      ManualControl::to_halt();
      delayMicroseconds(50);

      float adc_value = daq_.sample()[adc_channel];

      TEST_ASSERT_FLOAT_WITHIN(ANABRID_TESTS_TARGET_PRECISION, ic_value, -adc_value);
    }
  }

}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_ic);
  UNITY_END();
}

void loop() {}
