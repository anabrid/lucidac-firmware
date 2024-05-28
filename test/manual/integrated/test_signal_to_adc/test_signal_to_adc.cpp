// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace platform;
using namespace blocks;
using namespace daq;
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

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(cluster.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_direct_ref() {
  // Connect -1*REF to U-Block outputs 0 & 1
  TEST_ASSERT(cluster.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  for (auto out_to_adc : blocks::UBlock::OUTPUT_IDX_RANGE_TO_ADC()) {
    TEST_ASSERT(cluster.ublock->connect(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, out_to_adc));
  }
  cluster.write_to_hardware();

  // Measure result
  delayMicroseconds(100);
  auto data = daq_.sample();
  for (auto d : data) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, d);
  }

  cluster.ublock->reset(true);
  cluster.ublock->write_to_hardware();
}

void test_ic_to_adc() {
  auto *intblock = (MIntBlock *)(cluster.m1block);

  uint8_t integrator_idx = 0;
  uint8_t adc_chanel = 0;
  cluster.ublock->connect(MBlock::M1_OUTPUT(integrator_idx), adc_chanel);

  for (int i = -100; i <= 100; i++) {
    float value = static_cast<float>(i) / 100.0f;
    // Write config to hardware
    intblock->set_ic(integrator_idx, value);
    cluster.write_to_hardware();
    // Let integrator be charged to IC value
    ManualControl::to_ic();
    delayMicroseconds(1000);
    ManualControl::to_halt();
    // Read back value with ADC (except it was negated)
    auto data = daq_.sample();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -value, data[adc_chanel]);
  }

  cluster.ublock->reset(true);
  cluster.ublock->write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_direct_ref);
  RUN_TEST(test_ic_to_adc);
  UNITY_END();
}

void loop() {}
