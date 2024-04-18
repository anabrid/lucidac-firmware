// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "block/iblock.h"

using namespace blocks;

IBlock iblock{0};

void setUp() {
  // This is called before *each* test.
  iblock.reset(false);
}

void tearDown() {
  // This is called after *each* test.
}

void test_split_signal_constraint() {
  // Connect one input to an output
  TEST_ASSERT(iblock.connect(0,0));
  TEST_ASSERT(iblock.is_connected(0,0));
  // And add another input, which is fine
  TEST_ASSERT(iblock.connect(1,0));
  TEST_ASSERT(iblock.is_connected(1,0));

  // But it's usually not wanted to split inputs to multiple signals
  TEST_ASSERT_FALSE(iblock.connect(1,1));
  TEST_ASSERT(iblock.is_connected(0,0));
  TEST_ASSERT(iblock.is_connected(1,0));
  TEST_ASSERT_FALSE(iblock.is_connected(1,1));
  // Maybe for some future applications (oscillator coupling), it may be useful.
  // Use allow_input_splitting to allow connecting one input to multiple outputs.
  TEST_ASSERT(iblock.connect(1,1, false,true));
  TEST_ASSERT(iblock.is_connected(0,0));
  TEST_ASSERT(iblock.is_connected(1,0));
  TEST_ASSERT(iblock.is_connected(1,1));
}

void test_disconnect() {
  // Some connections
  TEST_ASSERT(iblock.connect(0,0));
  TEST_ASSERT(iblock.is_connected(0,0));
  TEST_ASSERT(iblock.connect(1,0));
  TEST_ASSERT(iblock.is_connected(1,0));

  // Disconnect one
  TEST_ASSERT(iblock.disconnect(0,0));
  TEST_ASSERT_FALSE(iblock.is_connected(0,0));
  TEST_ASSERT(iblock.is_connected(1,0));

  // Disconnect all
  TEST_ASSERT(iblock.disconnect(0));
  TEST_ASSERT_FALSE(iblock.is_connected(1,0));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_split_signal_constraint);
  RUN_TEST(test_disconnect);
  UNITY_END();
}

void loop() { delay(500); }
