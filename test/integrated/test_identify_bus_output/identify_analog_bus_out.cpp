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

    // Connect a sinusoidal solution
    for(int i=0; i<8; i++) {
      TEST_ASSERT(intblock->set_ic(i, 0.1 * i));
      TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(i), 0, -1.0f, MBlock::M1_INPUT(i)));
    }
    
  // after long and tedious search:
  luci.ublock->connect(MBlock::M1_OUTPUT(0), 15); // OUT0
  luci.ublock->connect(MBlock::M1_OUTPUT(1), 14); // OUT1
  luci.ublock->connect(MBlock::M1_OUTPUT(2), 13); // OUT2
  luci.ublock->connect(MBlock::M1_OUTPUT(3), 12); // OUT3
  luci.ublock->connect(MBlock::M1_OUTPUT(4), 11); // OUT4
  luci.ublock->connect(MBlock::M1_OUTPUT(5), 10); // OUT5
  luci.ublock->connect(MBlock::M1_OUTPUT(6),  9); // OUT6
  luci.ublock->connect(MBlock::M1_OUTPUT(7),  8); // OUT7
  //luci.ublock->connect(MBlock::M1_OUTPUT(0), 16); // OUT5

  luci.write_to_hardware();
  delayMicroseconds(100);
    
  mode::ManualControl::to_ic();
  delayMicroseconds(120);
  
  // NO OP!
  /*
    mode::ManualControl::to_op();
    delayMicroseconds(6666); //50*1000*1000);
    mode::ManualControl::to_halt();
    */
    
  
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
