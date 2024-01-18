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

#include "carrier/carrier.h"
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
  
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_single() {
  auto *multIdentityBlock = (MIntBlock *)(luci.m2block);


  luci.reset(true);
  TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  auto one = blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT;
  
  luci.write_to_hardware();
  delayMicroseconds(500);

  luci.route(one,0,1,multIdentityBlock->M2_INPUT(0));
  luci.route(one,20,1,multIdentityBlock->M2_INPUT(0));

  luci.write_to_hardware();
  
  delay(10);

  //luci.reset(true);
  //luci.write_to_hardware();
}

void test_sweep() {
  auto *multIdentityBlock = (MIntBlock *)(luci.m1block);
  auto one = blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT;

  for (uint8_t inlane; inlane < 32; inlane++) {
    luci.reset(true);
    TEST_ASSERT(luci.ublock->use_alt_signals(one));
    
    luci.write_to_hardware();
    delay(10);

    luci.route(one,inlane,1,multIdentityBlock->M1_INPUT(0));

    luci.write_to_hardware();
    
    delay(10);
  }



  luci.reset(true);
  luci.write_to_hardware();
}



void test_integrate() {
  auto *intBlock = (MIntBlock *)(luci.m1block);

  uint8_t intIndex = 3;

  luci.reset(true);
  TEST_ASSERT(luci.ublock->use_alt_signals(blocks::UBlock::ALT_SIGNAL_REF_HALF));
  auto one = blocks::UBlock::ALT_SIGNAL_REF_HALF_INPUT;
  
  luci.write_to_hardware();
  delayMicroseconds(500);

  luci.route(one,0,1,intBlock->M1_INPUT(intIndex));

  // this is right:
  luci.ublock->connect(intBlock->M1_OUTPUT(intIndex), UBlock::IDX_RANGE_TO_ACL_OUT(4)); // 8 oder 15
  luci.ublock->connect(intBlock->M1_OUTPUT(intIndex), UBlock::IDX_RANGE_TO_ACL_OUT(5)); // 9 oder 14
  luci.ublock->connect(intBlock->M1_OUTPUT(intIndex), UBlock::IDX_RANGE_TO_ACL_OUT(6)); // 10 oder 13
  luci.ublock->connect(intBlock->M1_OUTPUT(intIndex), UBlock::IDX_RANGE_TO_ACL_OUT(7)); // 11 oder 12

  luci.write_to_hardware();
  
  // Set IC and then let it integrate
  ManualControl::to_ic();
  delay(1);
  ManualControl::to_op();
  delay(10);
  ManualControl::to_halt();

  luci.reset(true);
  luci.write_to_hardware();
}

void test_sinus() {
  auto *intBlock1 = (MIntBlock *)(luci.m1block);
  auto *intBlock2 = (MIntBlock *)(luci.m1block);

  intBlock1->set_ic(0, 1);
  intBlock2->set_ic(1, 0);

  luci.reset(true);
  luci.write_to_hardware();
  delayMicroseconds(500);

  luci.route(intBlock1->M1_OUTPUT(0),0,1,intBlock2->M1_INPUT(1));
  luci.route(intBlock2->M1_OUTPUT(1),1,-1,intBlock1->M1_INPUT(0));

  // this is right:
  luci.ublock->connect(intBlock1->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(4)); // 8 oder 15
  luci.ublock->connect(intBlock1->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(5)); // 9 oder 14
  luci.ublock->connect(intBlock1->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(6)); // 10 oder 13
  luci.ublock->connect(intBlock1->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(7)); // 11 oder 12

  luci.write_to_hardware();
  
  // Set IC and then let it integrate
  for (;;){
    ManualControl::to_ic();
    delayMicroseconds(50);
    ManualControl::to_op();
    delayMicroseconds(1000);
    ManualControl::to_halt();
  }

  luci.reset(true);
  luci.write_to_hardware();
}

void test_sinus_damped() {
  auto *intBlock = (MIntBlock *)(luci.m1block);

  intBlock->set_ic(0, 1);
  intBlock->set_ic(1, 0);

  luci.reset(true);
  luci.write_to_hardware();
  delayMicroseconds(500);

  luci.route(intBlock->M1_OUTPUT(0),0,1,intBlock->M1_INPUT(1));
  luci.route(intBlock->M1_OUTPUT(0),1,-0.1,intBlock->M1_INPUT(0));
  luci.route(intBlock->M1_OUTPUT(1),2,-1,intBlock->M1_INPUT(0));

  // this is right:
  luci.ublock->connect(intBlock->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(4)); // 8 oder 15
  luci.ublock->connect(intBlock->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(5)); // 9 oder 14
  luci.ublock->connect(intBlock->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(6)); // 10 oder 13
  luci.ublock->connect(intBlock->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(7)); // 11 oder 12

  luci.write_to_hardware();
  
  // Set IC and then let it integrate
  for (;;){
    ManualControl::to_ic();
    delayMicroseconds(50);
    ManualControl::to_op();
    delayMicroseconds(100000);
    ManualControl::to_halt();
  }

  luci.reset(true);
  luci.write_to_hardware();
}

void test_roesler() {
  auto *mBlock = (MIntBlock *)(luci.m1block);

  luci.reset(true);
  luci.write_to_hardware();
  delayMicroseconds(500);

  mBlock->set_ic(0, 0.01);
  mBlock->set_ic(1, 0);
  mBlock->set_ic(2, 0);

  luci.route(mBlock->M1_OUTPUT(1),0,-0.9,mBlock->M1_INPUT(0));
  luci.route(mBlock->M1_OUTPUT(2),1,-2.9,mBlock->M1_INPUT(0));

  luci.route(mBlock->M1_OUTPUT(0),2,1.95,mBlock->M1_INPUT(1));
  luci.route(mBlock->M1_OUTPUT(1),3,0.2,mBlock->M1_INPUT(1));

  luci.route(mBlock->M2_OUTPUT(4),4,-0.1,mBlock->M1_INPUT(2));
  luci.route(mBlock->M2_OUTPUT(0),5,-9,mBlock->M1_INPUT(2));

  luci.route(mBlock->M1_OUTPUT(2),6,1.5,mBlock->M2_INPUT(0));
  luci.route(mBlock->M1_OUTPUT(0),7,10,mBlock->M2_INPUT(1));
  luci.route(mBlock->M2_OUTPUT(4),8,0.3,mBlock->M2_INPUT(1));

  // this is right:
  luci.ublock->connect(mBlock->M1_OUTPUT(0), UBlock::IDX_RANGE_TO_ACL_OUT(5)); // 8 oder 15
  luci.ublock->connect(mBlock->M1_OUTPUT(1), UBlock::IDX_RANGE_TO_ACL_OUT(6)); // 9 oder 14
  luci.ublock->connect(mBlock->M1_OUTPUT(2), UBlock::IDX_RANGE_TO_ACL_OUT(7)); // 10 oder 13

  luci.write_to_hardware();
  
  // Set IC and then let it integrate
  for (;;){
    ManualControl::to_ic();
    delay(1);
    ManualControl::to_op();
    delay(100);
    ManualControl::to_halt();
  }

  luci.reset(true);
  luci.write_to_hardware();
}


void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  //RUN_TEST(test_single);
  //RUN_TEST(test_sweep);
  //RUN_TEST(test_integrate);
  //RUN_TEST(test_sinus);
  //RUN_TEST(test_sinus_damped);
  RUN_TEST(test_roesler);
  UNITY_END();
}

void loop() {}
