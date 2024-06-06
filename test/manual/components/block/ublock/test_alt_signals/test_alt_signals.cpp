// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "block/ublock.h"
#include "bus/bus.h"

using namespace blocks;

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_function() {
  UBlock ublock;
  TEST_ASSERT_EQUAL(0, ublock.get_alt_signals());

  TEST_ASSERT(ublock.use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));
  TEST_ASSERT_EQUAL(UBlock::ALT_SIGNAL_REF_HALF, ublock.get_alt_signals());

  TEST_ASSERT_FALSE(ublock.use_alt_signals(UBlock::MAX_ALT_SIGNAL + 1));

  TEST_ASSERT(ublock.use_alt_signals(UBlock::ALT_SIGNAL_ACL7));
  TEST_ASSERT_EQUAL(UBlock::ALT_SIGNAL_REF_HALF | UBlock::ALT_SIGNAL_ACL7, ublock.get_alt_signals());

  ublock.write_alt_signal_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
