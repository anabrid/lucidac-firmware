// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
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
  TEST_ASSERT(cluster.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_function() {
  auto *intblock = (MIntBlock *)(cluster.m0block);

  // Connect a sinusoidal solution
  for (int i = 0; i < 8; i++) {
    TEST_ASSERT(intblock->set_ic(i, 0.1 * i));
    TEST_ASSERT(cluster.route(MBlock::M1_OUTPUT(i), 0, -1.0f, MBlock::M1_INPUT(i)));
  }

  // after long and tedious search:
  cluster.ublock->connect(MBlock::M1_OUTPUT(0), 15); // OUT0
  cluster.ublock->connect(MBlock::M1_OUTPUT(1), 14); // OUT1
  cluster.ublock->connect(MBlock::M1_OUTPUT(2), 13); // OUT2
  cluster.ublock->connect(MBlock::M1_OUTPUT(3), 12); // OUT3
  cluster.ublock->connect(MBlock::M1_OUTPUT(4), 11); // OUT4
  cluster.ublock->connect(MBlock::M1_OUTPUT(5), 10); // OUT5
  cluster.ublock->connect(MBlock::M1_OUTPUT(6), 9);  // OUT6
  cluster.ublock->connect(MBlock::M1_OUTPUT(7), 8);  // OUT7
  // cluster.ublock->connect(MBlock::M1_OUTPUT(0), 16); // OUT5

  cluster.write_to_hardware();
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
