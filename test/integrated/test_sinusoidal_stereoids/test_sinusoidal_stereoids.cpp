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
  TEST_MESSAGE("init");

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
  
  TEST_MESSAGE("Hello from test");
  
  // We need a +1 later
  TEST_ASSERT(luci.ublock->use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));
  //auto one = UBlock::ALT_SIGNAL_REF_HALF_INPUT;
  
  uint8_t one = 12; // Nehme 1 von Ausgang M-Block
  
  // choose integrators to use:
  uint8_t i0 = 0, i1 = 1, i2 = 2;
  // choose multiplier to use:
  uint8_t m0 = 0;

  // just to be 100% sure, we don't use magic here but the resulting numbers instead.  
  uint8_t
    mx_in  = 8+ 0, // MBlock::M1_INPUT(i0),
    mx_out = 8+ 0, // MBlock::M1_OUTPUT(i0),
    y_in   = 8+ 1, // MBlock::M1_INPUT(i1),
    y_out  = 8+ 1, // MBlock::M1_OUTPUT(i1),
    z_in   = 8+ 2, // MBlock::M1_INPUT(i2),
    z_out  = 8+ 2, //MBlock::M1_OUTPUT(i2),
    mult_a = 8 -8, //MBlock::M2_INPUT(2*m0),
    mult_b = 9 -8, //MBlock::M2_INPUT(2*m0+1),
    mult_o = 8 -8; // MBlock::M2_OUTPUT(m0);
  
  TEST_ASSERT(intblock->set_ic(i0, 0.066));
  TEST_ASSERT(intblock->set_ic(i1, 0.));
  TEST_ASSERT(intblock->set_ic(i2, 0.05));
  
  String msg;
  #define route(a,b,c,d) \
    TEST_ASSERT(luci.route(a,b,c,d)); \
    msg = String("Route(")+String(a)+String(", ")+String(b)+String(",")+String(c)+String(",")+String(d)+String(")"); \
    TEST_MESSAGE(msg.c_str() );
  
  // ACL_OUT ports
  uint8_t OUT0 = 8,
          OUT1 = 9,
          OUT2 = 10,
          OUT3 = 11;
  
  // This actually defines the Roesler/Roessler attractor
  route(mx_out, OUT0,   -1.25f,      y_in);
  route(y_out,  OUT1,     0.2f,      y_in);
  route(z_out,  OUT2,      1.f,    mult_b);
  route(mult_o, OUT3,    -15.f,      z_in);    
  route(mx_out,   16,     -1.f,    mult_a);
  route(one,       0, -0.3796f,    mult_a);
  route(y_out,     1,     0.8f,     mx_in);
  route(z_out,     2,     2.3f,     mx_in);
  route(one,       3,   0.005f,      z_in);

  luci.write_to_hardware();
  delayMicroseconds(100);
    
  for(;;) {
      
    mode::ManualControl::to_ic();
    delayMicroseconds(120);
    mode::ManualControl::to_op();
    delayMicroseconds(5*6666);
    mode::ManualControl::to_halt();
    delayMicroseconds(200); //50*1000*1000);

    TEST_MESSAGE("CYCLE");

  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
