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

#include "carrier/cluster.h"
#include "daq.h"
#include "mode.h"

using namespace lucidac;
using namespace blocks;
using namespace daq;
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

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_direct_ref() {
  // Connect -1*REF to U-Block outputs 0 & 1
  TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  for (auto out_to_adc : blocks::UBlock::OUTPUT_IDX_RANGE_TO_ADC()) {
    TEST_ASSERT(luci.ublock->connect(blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT, out_to_adc));
  }
  luci.write_to_hardware();

  // Measure result
  delayMicroseconds(100);
  auto data = daq_.sample();
  for (auto d: data) {
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, d);
  }

  luci.ublock->reset(true);
  luci.ublock->write_to_hardware();
}

void test_ic_to_adc() {
  auto *intblock = (MIntBlock *)(luci.m1block);

  uint8_t integrator_idx = 0;
  uint8_t adc_chanel = 0;
  luci.ublock->connect(MBlock::M1_OUTPUT(integrator_idx), adc_chanel);

  for (int i = -100; i <= 100; i++) {
    float value = static_cast<float>(i)/100.0f;
    // Write config to hardware
    intblock->set_ic(integrator_idx, value);
    luci.write_to_hardware();
    // Let integrator be charged to IC value
    ManualControl::to_ic();
    delayMicroseconds(1000);
    ManualControl::to_halt();
    // Read back value with ADC (except it was negated)
    auto data = daq_.sample();
    TEST_ASSERT_FLOAT_WITHIN(0.01f, -value, data[adc_chanel]);
  }

  luci.ublock->reset(true);
  luci.ublock->write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_direct_ref);
  RUN_TEST(test_ic_to_adc);
  UNITY_END();
}

void loop() {}
