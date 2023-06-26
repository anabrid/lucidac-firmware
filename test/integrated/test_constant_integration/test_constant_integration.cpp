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

#include "daq.h"
#include "lucidac.h"
#include "mode.h"

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
  auto* intblock = (MIntBlock*)(luci.m1block);

  auto coeff_idx_to_test = UBlock::OUTPUT_IDX_RANGE();
  //std::array<uint8_t,1> coeff_idx_to_test = {4};
  for (auto coeff_idx : coeff_idx_to_test) {
    char buffer[128] = {'C','_','i','d','x','='};
    itoa(coeff_idx, buffer + 6, 10);
    TEST_MESSAGE(buffer);

    uint8_t adc_channel = coeff_idx != 7 ? 7 : 0;

    // Reset
    luci.reset(true);
    delayMicroseconds(500);

    // Enable REF signals on U-Block
    TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
    // Route it once through to the integration module in M1 slot
    float factor = -1.0f;
    luci.route(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, coeff_idx, factor, MBlock::M1_INPUT(0));
    // Route the result to an ADC input
    luci.ublock->connect(MBlock::M1_OUTPUT(0), UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);
    // Define IC value
    float ic_value = 0.0f;
    intblock->set_ic(0, ic_value);
    // Write config to hardware
    luci.write_to_hardware();
    delayMicroseconds(100);

    // Check for correct output
    unsigned int op_time_us = 100;
    float expected_value = ic_value + factor * static_cast<float>(op_time_us) / 100.0f;
    float accepted_error = 0.05f;
    // Load IC and then let it integrate
    ManualControl::to_ic();
    delayMicroseconds(50);
    TEST_ASSERT_FLOAT_WITHIN(accepted_error, -ic_value, daq_.sample()[adc_channel]);
    ManualControl::to_op();
    delayMicroseconds(100);
    ManualControl::to_halt();
    TEST_ASSERT_FLOAT_WITHIN(accepted_error, -expected_value, daq_.sample()[adc_channel]);

    // Change factor to negative
    luci.cblock->set_factor(coeff_idx, -factor);
    luci.cblock->write_to_hardware();

    // Set IC and then let it integrate
    ManualControl::to_ic();
    delayMicroseconds(50);
    TEST_ASSERT_FLOAT_WITHIN(accepted_error, +ic_value, daq_.sample()[adc_channel]);
    ManualControl::to_op();
    delayMicroseconds(100);
    ManualControl::to_halt();
    TEST_ASSERT_FLOAT_WITHIN(accepted_error, +expected_value, daq_.sample()[adc_channel]);

    // Delay for better visibility on oscilloscope
    delay(500);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
