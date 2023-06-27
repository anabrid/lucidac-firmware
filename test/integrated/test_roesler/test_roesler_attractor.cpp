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
  auto *intblock = (MIntBlock *)(luci.m1block);

  // We need a +1 later
  TEST_ASSERT(luci.ublock->use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));

  // Choose integrators and set IC
  // An IC value's sign is actually equal to the output's sign (not inverted)
  struct { int  mx = MBlock::M1_OUTPUT(0),
                 y = MBlock::M1_OUTPUT(1),
                 z = MBlock::M1_OUTPUT(2),
              mult = MBlock::M2_OUTPUT(0); } out;
  struct { int  mx = MBlock::M1_INPUT(0),
                 y = MBlock::M1_INPUT(1),
                 z = MBlock::M1_INPUT(2),
              mult = MBlock::M2_INPUT(0); } in;
  auto one = UBlock::ALT_SIGNAL_REF_HALF; // +1
  TEST_ASSERT(intblock->set_ic(0, 0.666)); // mx
  TEST_ASSERT(intblock->set_ic(1,  0));    // y
  TEST_ASSERT(intblock->set_ic(2,  0));    // z
  
  int poti = 0;  
  
  #define patch(a, b, c) TEST_ASSERT(luci.route(a, poti++, b, c )) 
  
  patch(out.mx,   -1.25f,   in.y);
  patch(out.mx,    1.f,     in.mult);
  patch(one,       0.3796f, in.mult);
  patch(out.mult, 15.f,     in.z);
  patch(one,       0.005f,  in.z);
  patch(out.y,     0.2f,    in.y);
  patch(out.y,     0.8f,    in.mx);
  patch(out.z,     1.f,     in.mult);
  patch(out.y,     2.3f,    in.mx);
  
  // readout
  luci.ublock->connect(out.mx, 15); // OUT0
  luci.ublock->connect(out.y,  14); // OUT1
  luci.ublock->connect(out.z,  13); // OUT2
  luci.ublock->connect(out.z,  12); // OUT3

  // Write to hardware
  luci.write_to_hardware();
  delayMicroseconds(100);

  TEST_MESSAGE("Written to hardware, starting IC OP.");

  // Run it
  for(;;) {
  
  mode::ManualControl::to_ic();
  delayMicroseconds(120);
  mode::ManualControl::to_op();
  delayMicroseconds(1000*6666);
  mode::ManualControl::to_halt();

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
