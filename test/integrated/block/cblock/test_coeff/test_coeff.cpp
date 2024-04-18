// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace lucidac;
using namespace mode;

LUCIDAC luci{};
OneshotDAQ daq_{};

uint8_t coeff;
float tolerance = 0.02;

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

void test_coeff() {
  auto *identityblock = (MIntBlock *)(luci.m1block);

  float diff = 0;
  float sum_diff = 0;
  float max_diff = 0;
  uint32_t num_diff = 0;

  uint8_t adc_channel = coeff != 7 ? 7 : 0;

  // Reset
  luci.reset(true);
  delayMicroseconds(100);

  // Enable REF signals on U-Block
  TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  TEST_ASSERT(luci.ublock->connect(identityblock->M1_OUTPUT(0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]));

  // Route the result to an ACL output
  TEST_ASSERT(luci.ublock->connect(identityblock->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(5)));

  // Route identity block
  TEST_ASSERT(luci.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, coeff, 0, identityblock->M1_INPUT(0)));
  
  luci.write_to_hardware();
  delayMicroseconds(100);

  for (float c = -1; c <= 1; c += 0.001) {
    TEST_ASSERT(luci.cblock->set_factor(coeff,c));
    luci.cblock->write_to_hardware();
    delayMicroseconds(10);

    float adc_value = daq_.sample()[adc_channel];

    diff = abs(c + adc_value);
    sum_diff += diff;
    max_diff = diff > max_diff ? diff : max_diff;
    num_diff++;
  }
  
  char buffer[128];

  sprintf(buffer, "Coeff %d | ACD %d | Mean Diff = %f | Max Diff  = %f", coeff, adc_channel, sum_diff/num_diff, max_diff);
  TEST_MESSAGE(buffer);

  sprintf(buffer, "Maximal Difference for Coeff %d was %f", coeff, max_diff);
  TEST_ASSERT_MESSAGE(max_diff < tolerance, buffer);
}


void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  char buffer[128];
  sprintf(buffer, "Sweep through [-1, 1] for each Coefficient and check if actual value is within %f", tolerance);
  TEST_MESSAGE(buffer);
  for (coeff = 0; coeff < 32 ; coeff++) {
    RUN_TEST(test_coeff);
  }
  UNITY_END();
}

void loop() {}
