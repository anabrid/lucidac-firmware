// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "block/ublock.h"

using namespace blocks;

UBlock ublock;

void setUp() {
  // This is called before *each* test.
  ublock.reset(false);
}

void tearDown() {
  // This is called after *each* test.
}

void test_multiple_output_constraints() {
  // Connect one input to an output
  TEST_ASSERT(ublock.connect(0, 0));
  TEST_ASSERT(ublock.is_connected(0, 0));
  // Connect the same input to another output
  TEST_ASSERT(ublock.connect(0, 1));
  TEST_ASSERT(ublock.is_connected(0, 1));

  // Connecting another input to the same output would disconnect the previous ones
  // To reduce unintended errors, disconnecting other connections must explicitly be allowed
  // This connection will fail
  TEST_ASSERT_FALSE(ublock.connect(1, 0));
  TEST_ASSERT(ublock.is_connected(0, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
  // Allow disconnection to succeed
  TEST_ASSERT(ublock.connect(1, 0, true));
  TEST_ASSERT(ublock.is_connected(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(0, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));

  // Try to unsuccessfully overwrite again
  TEST_ASSERT_FALSE(ublock.connect(2, 0));
  TEST_ASSERT(ublock.is_connected(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(2, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
  // Clean up by disconnecting first
  TEST_ASSERT(ublock.disconnect(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(2, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
  // Then we can connect without any disconnections
  TEST_ASSERT(ublock.connect(2, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(1, 0));
  TEST_ASSERT(ublock.is_connected(2, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
}

void test_alt_signal_connect() {
  // After reset, no alt signals are used
  TEST_ASSERT_FALSE(ublock.get_alt_signals());
  TEST_ASSERT_FALSE(ublock.is_alt_signal_used(UBlock::ALT_SIGNAL_REF_HALF));

  // Connecting an alt signal automatically enables it
  TEST_ASSERT(ublock.connect_alt_signal(UBlock::ALT_SIGNAL_REF_HALF, 0));
  TEST_ASSERT(ublock.is_alt_signal_used(UBlock::ALT_SIGNAL_REF_HALF));
  TEST_ASSERT(ublock.is_connected(UBlock::ALT_SIGNAL_REF_HALF_INPUT, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(UBlock::ALT_SIGNAL_REF_HALF_INPUT, 1));
  // Connecting to another output should work just the same
  TEST_ASSERT(ublock.connect_alt_signal(UBlock::ALT_SIGNAL_REF_HALF, 1));
  TEST_ASSERT(ublock.is_alt_signal_used(UBlock::ALT_SIGNAL_REF_HALF));
  TEST_ASSERT(ublock.is_connected(UBlock::ALT_SIGNAL_REF_HALF_INPUT, 0));
  TEST_ASSERT(ublock.is_connected(UBlock::ALT_SIGNAL_REF_HALF_INPUT, 1));

  // Some signals can not be connected to anywhere
  TEST_ASSERT_FALSE(ublock.connect_alt_signal(UBlock::ALT_SIGNAL_ACL0, 15));
  // This should not enable the alt signal
  TEST_ASSERT_FALSE(ublock.is_alt_signal_used(UBlock::ALT_SIGNAL_ACL0));
  // Successfully connecting it should enable the alt signal
  TEST_ASSERT(ublock.connect_alt_signal(UBlock::ALT_SIGNAL_ACL0, 16));
  TEST_ASSERT(ublock.is_alt_signal_used(UBlock::ALT_SIGNAL_ACL0));
  // TODO: Yes, ALT_SIGNAL_ACL0 on input 8 is now connected to 16 via the second chip, but we may still use
  //       input 8 on the first chip for its original BL_IN8 signal. We ignore that for now.
  TEST_ASSERT(ublock.is_connected(8, 16));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_multiple_output_constraints);
  RUN_TEST(test_alt_signal_connect);
  UNITY_END();
}

void loop() { delay(500); }
