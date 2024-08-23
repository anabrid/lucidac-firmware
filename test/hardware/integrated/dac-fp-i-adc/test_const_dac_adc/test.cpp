// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#include "protocol/jsonl_logging.h"
#include "test_common.h"
#include "test_parametrized.h"

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "daq/daq.h"
#include "lucidac/lucidac.h"

#include <iostream>

/**
 * How to use this test:
 *
 * - Use LUCIDAC with M1 = M-Block-ID
 *   (TODO: Extend test to be able to use any MMul/M-ID block at any M1/M0)
 *
 * - Patch MCX MANUALLY on Front panel:
 *   - Connect AUX0 to IN0
 *   - Connect AUX1 to IN1
 *
 * Typical output:
 *
 * DAC0 input: 0.75 Expected output: -0.0125 Raw output: 8110 ADC0 output: 0.0124367
 * DAC1 input: -0.25 Expected output: 0.0375 Raw output: 8437 ACD1 output: -0.0374626
 * DAC0 input: -1 Expected output: 0.05 Raw output: 8519 ADC0 output: -0.0496703
 * DAC1 input: 1 Expected output: -0.05 Raw output: 7867 ACD1 output: 0.0496704
 * DAC0 input: -0.5 Expected output: -0.025 Raw output: 8032 ADC0 output: 0.0244918
 * DAC1 input: -0.5 Expected output: -0.025 Raw output: 8033 ACD1 output: 0.0244918
 * test/hardware/integrated/dac-fp-i-adc/test_const_dac_adc/test.cpp:114:test_dac_adc_values:PASS
 *
 **/

using namespace platform;

LUCIDAC lucidac;
daq::OneshotDAQ daq_;

void test_init_and_blocks() {
  // In carrier_.init(), missing blocks are ignored
  TEST_ASSERT(lucidac.init());
  // We do need certain blocks
  for (auto &cluster : lucidac.clusters) {
    TEST_ASSERT_NOT_NULL(cluster.iblock);
    TEST_ASSERT_NOT_NULL(cluster.shblock);
  }
  TEST_ASSERT_NOT_NULL(lucidac.ctrl_block);
  TEST_ASSERT_NOT_NULL(lucidac.front_panel);
  // Reset
  lucidac.reset(false);
  TEST_ASSERT(lucidac.write_to_hardware());
}

void test_dac_adc(float val0 = 0.5, float val1 = -0.25) {
  // The current implementation of the front plane outputs the value as voltage,
  // not in machine units (scaled by two), so we do it here for now.
  // TODO: Fix in front plane
  TEST_ASSERT(lucidac.front_panel->signal_generator.set_dac_out0(2 * val0));
  TEST_ASSERT(lucidac.front_panel->signal_generator.set_dac_out1(2 * val1));

  TEST_ASSERT(lucidac.set_acl_select(0, platform::LUCIDAC::ACL::EXTERNAL_));
  TEST_ASSERT(lucidac.set_acl_select(1, platform::LUCIDAC::ACL::EXTERNAL_));

  uint8_t acl0_lane = 24;
  uint8_t acl1_lane = 25;

  // Connect ACL to I-Block outputs
  uint8_t adc_channel_val0 = 0;
  uint8_t adc_channel_val1 = 1;
  TEST_ASSERT(lucidac.clusters[0].iblock->connect(acl0_lane, adc_channel_val0));
  TEST_ASSERT(lucidac.clusters[0].iblock->connect(acl1_lane, adc_channel_val1));

  // Prepare measure data via SH-Block
  TEST_ASSERT(lucidac.ctrl_block->set_adc_bus_to_cluster_gain(0));

  // Write configuration to hardware
  TEST_ASSERT_EQUAL(1, lucidac.write_to_hardware());

  // Measure data via SH-Block
  auto data = measure_sh_gain(lucidac.clusters[0], &daq_);

  float measured_val0 = data[adc_channel_val0];
  float measured_val1 = data[adc_channel_val1];
  std::cout << data << std::endl;

  // TODO: Improve, this is quite a lot of absolute tolerance.
  // I think 5% relative tolerance should do it.
  float abs_tolerance = 0.1;
  TEST_ASSERT_FLOAT_WITHIN(abs_tolerance, val0, measured_val0);
  TEST_ASSERT_FLOAT_WITHIN(abs_tolerance, val1, measured_val1);
}

void setup() {
  msg::activate_serial_log();
  bus::init();
  daq_.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_PARAM_TEST(test_dac_adc);
  RUN_PARAM_TEST(test_dac_adc, -1.0f, +1.0f);
  RUN_PARAM_TEST(test_dac_adc, -0.5f, -0.5f);
  UNITY_END();
}

void loop() { delay(500); }
