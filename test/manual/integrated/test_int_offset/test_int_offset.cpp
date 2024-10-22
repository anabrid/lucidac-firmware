// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace platform;
using namespace mode;

Cluster cluster{};
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

  // Put cluster start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(cluster.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.m0block, "M0-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(cluster.calibrate_routes(&daq_));
  delayMicroseconds(200);
}

void test_function() {
  auto *intblock = (MIntBlock *)(cluster.m0block);

  // Testergebnisse.ods

  /*
  // Choose which integrator [0..7] to use.
  uint8_t i0 = 0;
  float ic = 1;

  TEST_ASSERT(intblock->set_ic(i0, ic));

  // this combination of lanes (0 and 1) is known to work perfectly. Always.
  //TEST_ASSERT(cluster.route(one, ulane, +1.0f, MBlock::M0_INPUT(i0)));
  cluster.ublock->connect(MBlock::M0_OUTPUT(i0), 8); // ACL_OUT0
  cluster.ublock->connect(MBlock::M0_OUTPUT(i0), 7); // DAQ0


  cluster.write_to_hardware();
  delayMicroseconds(100);

  mode::ManualControl::to_ic();
  delayMicroseconds(2000);

  char buf[100]; sprintf(buf, "%f", daq_.sample()[7]);
  String msg = String("IC = ") + String(ic) + String(", ADC = ") + String(buf);
  TEST_MESSAGE(msg.c_str());
  */

  // Slope manually
  /*
  uint8_t i0 = 5;
  float ic = -1;
  TEST_ASSERT(intblock->set_ic(i0, ic));
  TEST_ASSERT(intblock->set_time_factor(i0, 100));
  // get the 0.5 refrence signal from an unused Output of the M1 Mult Block
  uint8_t one = MBlock::M1_OUTPUT(5);
  TEST_ASSERT(cluster.route(one, 16, +1.0f, MBlock::M0_INPUT(i0)));
  cluster.ublock->connect(MBlock::M0_OUTPUT(i0), 8); // ACL_OUT0
  cluster.ublock->connect(MBlock::M0_OUTPUT(i0), 0); // ADC0

  cluster.write_to_hardware();
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

  // slope automatically

  // k0 is either fast (10000, default) or slow (100)
  constexpr int k0 = 10000;
  constexpr int k0f = 1; // 1 for k0=10000, 100 for k0=100

  // candidate optimes in nanoseconds
  constexpr int optime_min = 150000 * k0f;
  constexpr int optime_max = 300000 * k0f;
  constexpr int step = 100 * k0f;
  constexpr int num = (optime_max - optime_min) / step;

  String msg = String("Sweeping all INTs over ") + String(num) + String(" candidates");
  TEST_MESSAGE(msg.c_str());

  for (;;) {
    msg = "";
    for (uint8_t i0 = 0; i0 < 8; i0++) {
      float ic = -1;
      uint8_t one = MBlock::M1_OUTPUT(5);
      cluster.reset(entities::ResetAction::CIRCUIT_RESET); // keep calibration
      TEST_ASSERT(cluster.route(one, 16, +1.0f, MBlock::M0_INPUT(i0)));
      TEST_ASSERT(cluster.ublock->connect(MBlock::M0_OUTPUT(i0), 8)); // ACL_OUT0
      TEST_ASSERT(cluster.ublock->connect(MBlock::M0_OUTPUT(i0), 0)); // ADC0
      TEST_ASSERT(intblock->set_time_factor(i0, k0));

      float current, last;
      int optime_optimal = -1;
      constexpr float target_integration_value = -1.0;

      for (int i = 0; i < num; i++) {
        last = current;
        TEST_ASSERT(intblock->set_ic_value(i0, ic));
        cluster.write_to_hardware();
        delayMicroseconds(100);
        mode::ManualControl::to_ic();
        delayMicroseconds(100); // for k0=10.000
        // delayMicroseconds(100*100); // for k0=100
        mode::ManualControl::to_op();
        delayNanoseconds(optime_min + i * step); // for k0=10.000
        // delayMicroseconds( (optime_min + i*step) / 1000 ); // for k0=100, wegen Wertedarstellung
        mode::ManualControl::to_halt();
        current = daq_.sample()[0];

        if (abs(current) >= abs(target_integration_value) && abs(last) < abs(target_integration_value)) {
          optime_optimal = optime_min + i * step;
          break;
        }
      }

      msg += String(optime_optimal) + String(" ");
      /*
            int best_index = 0;

            for(int i=0; i<num; i++) {
              int optime = optime_min + i * step;
              // poor man's find_minimum(integration_values - 1);
              if(  abs(integration_values[i] - target_integration_value)
                <  abs(integration_values[best_index] - target_integration_value) ){
                      best_index = i;
              }
              //char buf[100]; sprintf(buf, "%f", integration_values[i]);
              //msg = String(optime) + String(": ") + String(buf);
              //TEST_MESSAGE(msg.c_str());
            }
            //msg = String("INT") + String(i0) + String(": ") + String(optime_min + best_index * step);
            msg += String(optime_min + best_index * step) + String(" ");
      */
    }
    TEST_MESSAGE(msg.c_str());
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
