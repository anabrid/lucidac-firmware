// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "block/mblock.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace mode;

MIntBlock intblock{MBlock::SLOT::M0};
OneshotDAQ daq_{};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Initialize bus
  bus::init();
  // Initialize mode controller (currently separate thing)
  ManualControl::init();
  // Initialize MIntBlock
  TEST_ASSERT(intblock.init());
}

void test_function() {
  ManualControl::to_ic();
  intblock.set_ic(0, +1.0f);
  intblock.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
