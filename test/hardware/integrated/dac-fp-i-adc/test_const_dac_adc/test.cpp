// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define ANABRID_PEDANTIC

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

void test_dac_adc(float val0 = 0.75, float val1 = -0.25) {
  TEST_ASSERT(lucidac.front_panel != nullptr);

  TEST_ASSERT(lucidac.front_panel->signal_generator.set_dac_out0(val0));
  TEST_ASSERT(lucidac.front_panel->signal_generator.set_dac_out1(val1));

  TEST_ASSERT(lucidac.set_acl_select(0, platform::LUCIDAC::ACL::EXTERNAL_));
  TEST_ASSERT(lucidac.set_acl_select(1, platform::LUCIDAC::ACL::EXTERNAL_));

  uint8_t acl0_lane = 24;
  uint8_t acl1_lane = 25;

  // TODO: Detect where we have an identity connection in the system!
  //       I.e. detect on M-Mul or M-ID block and select suitable cross lanes!

  // this most likely only applies for M-ID block, not M-Mul which has gain 1.
  float id_gain = 1./20;

  // The following by purpose swaps val0 and val1 because it is known that
  // DAC1 and DAC0 are wrongly labeled...
  float expected_val0 = id_gain * val1;
  float expected_val1 = id_gain * val0;

  // This assumes an M-ID block in slot M1
  uint8_t m1_id0_in = 8, m1_id0_out = 8;
  uint8_t m1_id1_in = 9, m1_id1_out = 9;

  lucidac.clusters[0].iblock->connect(acl0_lane, m1_id0_in);
  lucidac.clusters[0].iblock->connect(acl1_lane, m1_id1_in);

  uint8_t adc_channel_val0 = 0;
  uint8_t adc_channel_val1 = 1;

  TEST_ASSERT(lucidac.set_adc_channel(adc_channel_val0, m1_id0_out));
  TEST_ASSERT(lucidac.set_adc_channel(adc_channel_val1, m1_id1_out));

  // write all at once: front panel, iblock, acl_select and adc_channels
  TEST_ASSERT_EQUAL(1, lucidac.write_to_hardware());

  // default value, was set by lucidac.reset().
  TEST_ASSERT(lucidac.ctrl_block->get_adc_bus() == blocks::CTRLBlock::ADCBus::ADC);

  delay(10);

  auto daq = daq::OneshotDAQ();
  TEST_ASSERT(daq.init(0));

  auto channels_raw = daq.sample_raw();
  auto channels = daq.sample();

  float measured_val0 = channels[adc_channel_val0];
  float measured_val1 = channels[adc_channel_val1];

  std::cout << " DAC0 input: " << val0
            << " Expected output: " << expected_val0
            << " Raw output: " << channels_raw[0]
            << " ADC0 output: " << measured_val0
            << std::endl;
  std::cout << " DAC1 input: " << val1
            << " Expected output: " << expected_val1
            << " Raw output: " << channels_raw[1]
            << " ACD1 output: " << measured_val1
            << std::endl;

  // TODO: Improve, this is quite a lot of absolute tolerance.
  // I think 5% relative tolerance should do it.
  float abs_tolerance = 0.1;

  TEST_ASSERT( fabs(expected_val0 - measured_val0) < abs_tolerance );
  TEST_ASSERT( fabs(expected_val1 - measured_val1) < abs_tolerance );
}

void test_dac_adc_values() {
  test_dac_adc();
  test_dac_adc(-1.0, +1.0);
  test_dac_adc(-0.5, -0.5);
}

void setup() {
  UNITY_BEGIN();

  bus::init();
  TEST_ASSERT(lucidac.init());

  lucidac.reset(false);
  RUN_TEST(test_dac_adc_values);

  UNITY_END();
}

void loop() { delay(500); }
