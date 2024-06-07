// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <unity.h>

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

void test() {
  TEST_ASSERT_EQUAL(0b0000'0000,
                    ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT));
  TEST_ASSERT_EQUAL(0b0000'0010,
                    ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF));
  TEST_ASSERT_EQUAL(0b0000'0100,
                    ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF));
  TEST_ASSERT_EQUAL(0b0000'0011,
                    ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF));
  TEST_ASSERT_EQUAL(0b0000'0101,
                    ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF));
  TEST_ASSERT_EQUAL(0b0000'0110, ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::GROUND));

  // reset transmission mode to 0, to simplify testing the alternative output
  TEST_ASSERT_EQUAL(0b0000'0000,
                    ublock.change_a_side_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT));

  TEST_ASSERT_EQUAL(0b0000'0000,
                    ublock.change_b_side_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT));
  TEST_ASSERT_EQUAL(0b0000'1000,
                    ublock.change_b_side_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF));
  TEST_ASSERT_EQUAL(0b0001'0000,
                    ublock.change_b_side_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF));
  TEST_ASSERT_EQUAL(0b0000'1001,
                    ublock.change_b_side_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF));
  TEST_ASSERT_EQUAL(0b0001'0001,
                    ublock.change_b_side_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF));
  TEST_ASSERT_EQUAL(0b0001'1000, ublock.change_b_side_transmission_mode(UBlock::Transmission_Mode::GROUND));

  TEST_ASSERT_EQUAL(0b0000'0000,
                    ublock.change_all_transmission_modes(UBlock::Transmission_Mode::ANALOG_INPUT));
  TEST_ASSERT_EQUAL(0b0000'1010, ublock.change_all_transmission_modes(UBlock::Transmission_Mode::POS_BIG_REF));
  TEST_ASSERT_EQUAL(0b0001'0100,
                    ublock.change_all_transmission_modes(UBlock::Transmission_Mode::POS_SMALL_REF));
  TEST_ASSERT_EQUAL(0b0000'1011, ublock.change_all_transmission_modes(UBlock::Transmission_Mode::NEG_BIG_REF));
  TEST_ASSERT_EQUAL(0b0001'0101,
                    ublock.change_all_transmission_modes(UBlock::Transmission_Mode::NEG_SMALL_REF));
  TEST_ASSERT_EQUAL(0b0001'1110, ublock.change_all_transmission_modes(UBlock::Transmission_Mode::GROUND));
}

int main(int argc, char **argv) {
  // The native platform does not use setup/loop functions, but a single main() function
  // When using "debug_test = native/examples/test_debugging" in platformio.ini, the debugger will start here.
  UNITY_BEGIN();
  RUN_TEST(test);
  UNITY_END();
}
