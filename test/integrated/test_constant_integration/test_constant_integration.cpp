// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
#include "daq/daq.h"
#include "mode/mode.h"

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

void test_function() {
  auto *intblock = (MIntBlock *)(luci.m1block);
  
  // Not ending this test can be handy when looking constantly
  // on the DSO and not having ideal trigger conditions.
  bool loop_forever = true;

  for(;loop_forever == true;) {
  
  // i0 loops over the eight integrators on M1 block, to test each of them
  for(uint8_t i0=0; i0<8; i0++) {

    i0=0; // fix the integrator to be tested

    auto coeff_idx_to_test = UBlock::OUTPUT_IDX_RANGE();
    //std::array<uint8_t,1> coeff_idx_to_test = {4};
    for (auto coeff_idx : coeff_idx_to_test) {

      coeff_idx = 0; // fix the coefficient to be tested

      char buffer[128];
      sprintf(buffer, "Int%d, C_idx=%d", i0, coeff_idx);
      TEST_MESSAGE(buffer);

    /* if (coeff_idx == 14) {
        TEST_MESSAGE("SKIPPED");
        continue;
      }*/

      uint8_t adc_channel = coeff_idx != 7 ? 7 : 0;

      // Reset
      luci.reset(true);
      delayMicroseconds(500);

      // Enable REF signals on U-Block
      TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
      // Route it once through to the integration module in M1 slot
      float factor = 0.5f;
      luci.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, coeff_idx, factor, MBlock::M1_INPUT(i0));
      // Route the result to an ADC input
      luci.ublock->connect(MBlock::M1_OUTPUT(i0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);
      // Define IC value
      float ic_value = 0.0f;
      intblock->set_ic(i0, ic_value);
      // Write config to hardware
      luci.write_to_hardware();
      delayMicroseconds(100);

      // Check for correct output
      unsigned int op_time_us = 20;
      float expected_value = ic_value + factor * static_cast<float>(op_time_us) / 100.0f;
      float accepted_error = 0.15f;
      // Load IC and then let it integrate
      ManualControl::to_ic();
      delayMicroseconds(50);
      TEST_ASSERT_FLOAT_WITHIN(accepted_error, -ic_value, daq_.sample()[adc_channel]);
      ManualControl::to_op();
      delayMicroseconds(op_time_us);
      ManualControl::to_halt();
      //TEST_ASSERT_FLOAT_WITHIN(accepted_error, -expected_value, daq_.sample()[adc_channel]);
      if (abs(-expected_value - daq_.sample()[adc_channel]) > accepted_error) {
        TEST_MESSAGE("FAILED");
      }

      // Change factor to negative
      luci.cblock->set_factor(coeff_idx, -factor);
      luci.cblock->write_to_hardware();

      // Set IC and then let it integrate
      ManualControl::to_ic();
      delayMicroseconds(50);
      TEST_ASSERT_FLOAT_WITHIN(accepted_error, +ic_value, daq_.sample()[adc_channel]);
      ManualControl::to_op();
      delayMicroseconds(op_time_us);
      ManualControl::to_halt();
      delayMicroseconds(20);
      
      //TEST_ASSERT_FLOAT_WITHIN(accepted_error, +expected_value, daq_.sample()[adc_channel]);
      if (abs(+expected_value - daq_.sample()[adc_channel]) > accepted_error) {
        TEST_MESSAGE("FAILED");
      }

      // Delay for better visibility on oscilloscope
      //delay(150);
    } // end for coeff index

  } // end for i0
  
    delay(100);
  } // end forever
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
