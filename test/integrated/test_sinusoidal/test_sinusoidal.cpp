// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "cluster.h"
#include "daq.h"
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

    // Choose which pair of integrators [0..7] to use. The port indices i0 and i1 correspond
    // directly to the input and output of the M1 block.
    uint8_t i0 = 0, i1 = 1;
    //uint8_t i0 = 2, i1 = 3;
    //uint8_t i0 = 4, i1 = 5;
    //uint8_t i0 = 6, i1 = 7;
  
    // IC for an oscillator should be a pair like {0,1} or {0,a} with some a.
    TEST_ASSERT(intblock->set_ic(i0, 0.5));
    TEST_ASSERT(intblock->set_ic(i1, 0));
    
    // For testing the different U-lanes:
    // Choose which U-lane to use (column in U-C-I blocks)
    uint8_t ustart_lane = 0;
    
    /*
    for(;;) {
      
      for(uint8_t ustart_lane=1; ustart_lane < 31; ustart_lane++) {
        if(ustart_lane == 9) ustart_lane ++;
        
        String msg = String("Start lane = ") + String(ustart_lane);
        TEST_MESSAGE(msg.c_str());
        
      //  luci.ublock->reset(true);
      //  luci.iblock->reset(true);
*/
        
        // this combination of lanes (0 and 1) is known to work perfectly. Always.
        TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(i0), 0,     +1.0f, MBlock::M1_INPUT(i1)));
        TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(i1), 1,     -1.0f, MBlock::M1_INPUT(i0)));
        

        // Enable this line for a damped oscillation
        //TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(i1), 2, -0.1f, MBlock::M1_INPUT(i1))); // damping
      
        /*
        // for testing the route additionally throught some multiplier, test these lines:
        // choose which multiplier to use, between 0 and 3
        uint8_t m0 = 3;
        TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(i0), 2, +1.0f, MBlock::M2_INPUT(2*m0)));
        TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(i0), 3, +1.0f, MBlock::M2_INPUT(2*m0 + 1)));
        */
        
        /*
        if(ustart_lane != 8) {
          luci.ublock->connect(MBlock::M1_OUTPUT(i0), 8); // OUT0
        }
        */
        
        luci.ublock->connect(MBlock::M1_OUTPUT(i0), 8); // ACL_OUT0
        luci.ublock->connect(MBlock::M1_OUTPUT(i1), 9); // ACL_OUT1
          
        luci.write_to_hardware();
        delayMicroseconds(100);
        
        for(;;) {
        
            mode::ManualControl::to_ic();
            delayMicroseconds(120);
            mode::ManualControl::to_op();
            delayMicroseconds(5000); //50*1000*1000);
            mode::ManualControl::to_halt();
        }

        /*
        ustart_lane--; // keep the same ulane for the next run
      }
    
    delayMicroseconds(1000*1000);
    }
    }*/
  
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
