// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <unity.h>

#define private public
#define protected public
#include "block/ublock.h"

using namespace blocks;

UBlock ublock{0};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_a_test() {
  TEST_ASSERT_EQUAL(0b0000'0000, ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                                 UBlock::Transmission_Target::REGULAR));
  TEST_ASSERT_EQUAL(0b0000'0010, ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF,
                                                                 UBlock::Transmission_Target::REGULAR));
  TEST_ASSERT_EQUAL(0b0000'0100, ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF,
                                                                 UBlock::Transmission_Target::REGULAR));
  TEST_ASSERT_EQUAL(0b0000'0011, ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF,
                                                                 UBlock::Transmission_Target::REGULAR));
  TEST_ASSERT_EQUAL(0b0000'0101, ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF,
                                                                 UBlock::Transmission_Target::REGULAR));
  TEST_ASSERT_EQUAL(0b0000'0110, ublock.change_transmission_mode(UBlock::Transmission_Mode::GROUND,
                                                                 UBlock::Transmission_Target::REGULAR));

  // reset transmission mode to 0, to simplify testing the alternative output
  TEST_ASSERT_EQUAL(0b0000'0000, ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                                 UBlock::Transmission_Target::REGULAR));

  TEST_ASSERT_EQUAL(0b0000'0000, ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                                 UBlock::Transmission_Target::ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0000'1000, ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF,
                                                                 UBlock::Transmission_Target::ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0001'0000, ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF,
                                                                 UBlock::Transmission_Target::ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0000'1001, ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF,
                                                                 UBlock::Transmission_Target::ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0001'0001, ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF,
                                                                 UBlock::Transmission_Target::ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0001'1000, ublock.change_transmission_mode(UBlock::Transmission_Mode::GROUND,
                                                                 UBlock::Transmission_Target::ALTERNATIVE));

  TEST_ASSERT_EQUAL(0b0000'0000,
                    ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0000'1010,
                    ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0001'0100,
                    ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0000'1011,
                    ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0001'0101,
                    ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE));
  TEST_ASSERT_EQUAL(0b0001'1110,
                    ublock.change_transmission_mode(UBlock::Transmission_Mode::GROUND,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE));
}

int main(int argc, char **argv) {
  // The native platform does not use setup/loop functions, but a single main() function
  // When using "debug_test = native/examples/test_debugging" in platformio.ini, the debugger will start here.
  UNITY_BEGIN();
  RUN_TEST(test_a_test);
  UNITY_END();
}
