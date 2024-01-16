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

  // Enable REF signals on U-Block
  TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  auto one = blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT;
  
  // Choose the multiplier to use, between [0,3]
  uint8_t mul = 0;

  // Test values to multiply (a*b = c)
  float a = 1.0f, b = 0.5f;

  // Ports for a*b = c
  uint8_t mul_a = 2*MBlock::M2_INPUT (mul),
          mul_b = 2*MBlock::M2_INPUT (mul) + 1,
          mul_c =   MBlock::M2_OUTPUT(mul);

  char msgbuf[100];
  sprintf(msgbuf, "%d  %d", mul_a, mul_b);
  TEST_MESSAGE(msgbuf);

  /* ein anderer Test */
  /*
  for(int j=0; j<31; j++) {
    luci.route(one, j, 0.5, mul_a);
  }
  */
  int Nfails = 0;
  int Ntotal = 0;
  // Teste alle Spaltenkombinationen durch den Rechner durch (eine "lane" geht jeweils durch U-C-I Block).
  for(uint8_t alane = 0; alane < 32; alane++)
  for(uint8_t blane = 0; blane < 32; blane++) {

  if(alane == blane) continue; // a*a can not be mapped this way onto Lucidac

  luci.ublock->reset(true); // important!
  luci.iblock->reset(true); // important!

  // Manually set the values to test, overwriting the loop logic.
  //alane = 0; // 1;
  //blane = 1; // 2;

  luci.route(one, alane, a, mul_a);
  luci.route(one, blane, b, mul_b);

  // decide which ADC channel ADC[0-7] to use in a non-colliding way
  uint8_t adc_channel = 0;
  while(alane == adc_channel || blane == adc_channel) {
    adc_channel++;
    adc_channel = adc_channel%7;
  }
  luci.ublock->connect(mul_c, UBlock::OUTPUT_IDX_RANGE_TO_ADC()[adc_channel]);

  // Connect ACL_OUT0 read out on port 8 only if non colliding.
  if(alane != 8 && blane != 8)
    luci.ublock->connect(mul_c, 8);

  // Write config to hardware
  luci.write_to_hardware();

  // integrator mode actually irrelevant but spikes are nice for triggering on oscilloscope
  delayMicroseconds(1000);
  ManualControl::to_ic();
  delayMicroseconds(50);

  // Check for correct output. Mind the negating multiplication on LUCIDAC.
  float expected_value = -a * b, actual_value = daq_.sample()[adc_channel];
  float accepted_relative_error = 0.15f;
  float accepted_absolute_error = 0.05f;
  bool failed = abs(expected_value - actual_value) > accepted_absolute_error;
  //bool failed = abs(expected_value - actual_value) / abs(expected_value) > accepted_relative_error;

  Ntotal++;
  if (failed) {
    Nfails++;
  }

  delayMicroseconds(1000);

  // TEST_ASSERT_FLOAT_WITHIN(accepted_error, -ic_value, daq_.sample()[adc_channel]);
  // TEST_ASSERT(!failed);

  // Print out debugging statements
  if (failed) {
    char msgbuf[100];
    sprintf(msgbuf, "MUL%d via alane,blane,adc=%2d,%2d,%2d: %.3f * %.3f = %.3f %s",
      mul, alane, blane, adc_channel, a, b, actual_value,  failed ? "FAILED" : "CORRECT" );
    TEST_MESSAGE(msgbuf);
  }
  //delayMicroseconds(10*1000*1000);


  }  //endfor
  sprintf(msgbuf, "Total number of Fails: %d/%d",
      Nfails, Ntotal);
    TEST_MESSAGE(msgbuf);

}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
