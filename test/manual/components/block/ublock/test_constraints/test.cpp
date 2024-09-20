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
  ublock.reset(entities::ResetAction::CIRCUIT_RESET);
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

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_multiple_output_constraints);
  UNITY_END();
}

void loop() { delay(500); }
