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

#define TEST_PRINTF(...) { char buf[200]; sprintf(buf, __VA_ARGS__); TEST_MESSAGE(buf); }

void test_function() {
  auto *intblock = (MIntBlock *)(luci.m1block);


  // Integrator to use [0..7]
  uint8_t i0 = 0;

  // coefficient to test
  uint8_t clane = 0;

  // adc to use
  uint8_t adc_idx = clane == 0 ? 1 : 0;


  // Slope manually
  /*
  uint8_t i0 = 5;
  float ic = -1;
  TEST_ASSERT(intblock->set_ic(i0, ic));
  TEST_ASSERT(intblock->set_time_factor(i0, 100));
  // get the 0.5 refrence signal from an unused Output of the M2 Mult Block
  uint8_t one = MBlock::M2_OUTPUT(5);
  TEST_ASSERT(luci.route(one, 16, +1.0f, MBlock::M1_INPUT(i0)));
  luci.ublock->connect(MBlock::M1_OUTPUT(i0), 8); // ACL_OUT0
  luci.ublock->connect(MBlock::M1_OUTPUT(i0), 0); // ADC0

  luci.write_to_hardware();
  delayMicroseconds(100);

  mode::ManualControl::to_ic();
  delayMicroseconds(2000);

  char buf[100]; sprintf(buf, "%f", daq_.sample()[0]);
  String msg = String("IC = ") + String(ic) + String(", ADC = ") + String(buf);
  TEST_MESSAGE(msg.c_str());

  for(;;) {
    mode::ManualControl::to_ic();
    delayMicroseconds(100*100);
    mode::ManualControl::to_op();
    //delayNanoseconds((206*1000 + 500)*100);
    delayMicroseconds(206*100 + 50);
    mode::ManualControl::to_halt();
    delayMicroseconds(100 * 100);
    //delayMicroseconds(3*1000*1000);
  }
  */


  TEST_MESSAGE("i0,clane,coeff_val,optime,abs_err_int_error");
  for(float coeff_val = 0.1; coeff_val < 1.0; coeff_val += 0.1) {
    luci.reset(true); // keep calibration

    //coeff_val = 0.2;
    // 767488  for 0.1

    // "higher quality" source of reference signal
    TEST_ASSERT(luci.ublock->use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));
    auto one = UBlock::ALT_SIGNAL_REF_HALF_INPUT;

    const float ic = 0.0;
    TEST_ASSERT(intblock->set_ic(i0, ic));
    TEST_ASSERT(luci.route(one, clane, coeff_val, MBlock::M1_INPUT(i0)));
    TEST_ASSERT(luci.ublock->connect(MBlock::M1_OUTPUT(i0), adc_idx));
    TEST_ASSERT(luci.ublock->connect(MBlock::M1_OUTPUT(i0), 8)); // ACL OUT1
    luci.write_to_hardware();
    delayMicroseconds(100);


    // scanning the half domain in order to avoid issues with ADC sign problems above analog unit
    const float target_integration_value = 0.5;

    // "informed" search, here is what we would expect:
    const uint32_t expected_optime = 204800 / 2 * (target_integration_value - ic) / coeff_val;

    const uint32_t optime_min = expected_optime * 0.5;
    const uint32_t optime_max = expected_optime * 1.5;
    const uint32_t max_steps = 1000;
    const uint32_t step_size = (optime_max - optime_min) / max_steps;

    // candidate optimes in microseconds
    // coeff=0.1 <-> 2ms runtime
    // coeff=20  <-> 10us runtime
    //uint32_t optime_min = 4*1000; // 8us
    //uint32_t optime_max = 8*1000*1000; // 3ms
    //uint32_t optime_cur;

    float intval_current, intval_last;
    uint32_t optime_current = 0;
  
    for(int i=0; i<max_steps; i++) {
      intval_last = intval_current;
      optime_current = optime_min + i * step_size;
      mode::ManualControl::to_ic();
      delayMicroseconds(100); // for k0=10.000
      mode::ManualControl::to_op();

//      if(optime_current > 1000*1000)
        delayMicroseconds(optime_current / 1000);
  //    else
    //    delayNanoseconds(optime_current);

      mode::ManualControl::to_halt();
      intval_current = -daq_.sample()[adc_idx];

      if(intval_current > target_integration_value) break;
      /*
      if( abs(intval_current) >= abs(target_integration_value) &&
          abs(intval_last)    <  abs(target_integration_value) ) {
          break;
      }
      */
    }

    TEST_PRINTF("%d,%d,%f,%ld", i0, clane, coeff_val, optime_current);
  }
 
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
