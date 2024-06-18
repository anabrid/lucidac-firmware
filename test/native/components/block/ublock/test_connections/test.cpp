// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <unity.h>
#include <Arduino.h>

#define private public
#define protected public
#include "block/ublock.h"

using namespace blocks;

UBlock ublock;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void reset_connections() {
  for (int i = 0; i < 32; i++) {
    TEST_ASSERT(ublock.disconnect(i));
  }
}

void test_connect_all() {
  ublock.change_all_transmission_modes(UBlock::GROUND);

  for (int i = 0; i < 16; i++) {
    TEST_ASSERT(ublock.connect(i, i));
  }
  for (int i = 0; i < 16; i++) {
    TEST_ASSERT(ublock.connect(i, i + 16));
  }

  TEST_ASSERT_FALSE(ublock.connect(1, 2));

  TEST_ASSERT_EQUAL(UBlock::ANALOG_INPUT, ublock.a_side_mode);
  TEST_ASSERT_EQUAL(UBlock::ANALOG_INPUT, ublock.b_side_mode);

  reset_connections();
}

void test_connect_alternative() {
  for (int i = 0; i < 10; i++) {
    TEST_ASSERT(ublock.connect(i, i));
  }

  TEST_ASSERT(ublock.connect_alternative(UBlock::GROUND, 10));
  TEST_ASSERT(ublock.is_connected(15, 10));

  TEST_ASSERT_EQUAL(UBlock::ANALOG_INPUT, ublock.a_side_mode);
  TEST_ASSERT_EQUAL(UBlock::GROUND, ublock.b_side_mode);

  TEST_ASSERT_FALSE(ublock.connect(15, 11));

  TEST_ASSERT_TRUE(ublock.connect(15, 11, true));
  TEST_ASSERT_EQUAL(UBlock::ANALOG_INPUT, ublock.b_side_mode);

  ublock.change_b_side_transmission_mode(UBlock::GROUND);

  TEST_ASSERT(ublock.connect_alternative(UBlock::GROUND, 20));
  TEST_ASSERT(ublock.is_connected(14, 20));

  ublock.change_b_side_transmission_mode(UBlock::POS_BIG_REF);
  TEST_ASSERT_FALSE(ublock.connect_alternative(UBlock::GROUND, 20));
}

int main(int argc, char **argv) {
  // The native platform does not use setup/loop functions, but a single main() function
  // When using "debug_test = native/examples/test_debugging" in platformio.ini, the debugger will start here.
  UNITY_BEGIN();
  RUN_TEST(test_connect_all);
  RUN_TEST(test_connect_alternative);
  UNITY_END();
}
