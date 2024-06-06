// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "block/ublock.h"

using namespace blocks;

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_block() {
  UBlock ublock;
  // So we have at least one concrete input signal
  ublock.use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF);

  TEST_ASSERT(ublock.connect(7, 0));
  TEST_ASSERT(ublock.connect(7, 5));
  TEST_ASSERT(ublock.connect(7, 10));
  TEST_ASSERT(ublock.connect(7, 15));
  TEST_ASSERT(ublock.connect(7, 20));
  TEST_ASSERT(ublock.connect(7, 25));
  TEST_ASSERT(ublock.connect(7, 30));
  ublock.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_block);
  UNITY_END();
}

void loop() {}
