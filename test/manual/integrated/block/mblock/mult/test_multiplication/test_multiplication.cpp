// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

#include "test_common.h"

using namespace blocks;
using namespace daq;
using namespace platform;
using namespace mode;

Cluster cluster{};
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

  // Put cluster start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(cluster.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.m1block, "M1-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(cluster.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_multiplication() {
  auto *multblock = (MIntBlock *)(cluster.m1block);
  uint8_t adc_channel = 7;

  for (uint8_t i0 = 0; i0 < 4; i0++) {
    char buffer[128];
    sprintf(buffer, "Mult%d ADC%d", i0, adc_channel);
    TEST_MESSAGE(buffer);

    // Reset
    cluster.reset(true);
    delayMicroseconds(500);

    // Enable REF signals on U-Block
    TEST_ASSERT(cluster.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
    cluster.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);

    if (ANABRID_TESTS_USE_ACL_OUT) {
      // Route the result to an ACL output
      cluster.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::IDX_RANGE_TO_ACL_OUT(5));
    }

    // Route Mult inputs
    cluster.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, 0, 0, multblock->M1_INPUT(2 * i0));
    cluster.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, 1, 0, multblock->M1_INPUT(2 * i0 + 1));

    cluster.write_to_hardware();
    delayMicroseconds(100);

    float factors[] = {-1, -0.5, 0, 0.5, 1};

    for (float a : factors) {
      for (float b : factors) {
        cluster.cblock->set_factor(0, a);
        cluster.cblock->set_factor(1, b);
        cluster.cblock->write_to_hardware();
        delayMicroseconds(100);

        float adc_value = daq_.sample()[adc_channel];

        TEST_ASSERT_FLOAT_WITHIN(ANABRID_TESTS_TARGET_PRECISION, a * b, -adc_value);

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
