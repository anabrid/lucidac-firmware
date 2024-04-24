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
  // Yes this code is formated
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                    UBlock::Transmission_Target::REGULAR),
                    0b0000'0000);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF,
                                                    UBlock::Transmission_Target::REGULAR),
                    0b0000'0010);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF,
                                                    UBlock::Transmission_Target::REGULAR),
                    0b0000'0100);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF,
                                                    UBlock::Transmission_Target::REGULAR),
                    0b0000'0011);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF,
                                                    UBlock::Transmission_Target::REGULAR),
                    0b0000'0101);
  TEST_ASSERT_EQUAL(
      ublock.change_transmission_mode(UBlock::Transmission_Mode::GROUND, UBlock::Transmission_Target::REGULAR),
      0b0000'0110);

  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                    UBlock::Transmission_Target::ALTERNATIVE),
                    0b0000'0000);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF,
                                                    UBlock::Transmission_Target::ALTERNATIVE),
                    0b0000'1000);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF,
                                                    UBlock::Transmission_Target::ALTERNATIVE),
                    0b0001'0000);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF,
                                                    UBlock::Transmission_Target::ALTERNATIVE),
                    0b0000'1001);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF,
                                                    UBlock::Transmission_Target::ALTERNATIVE),
                    0b0001'0001);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::GROUND,
                                                    UBlock::Transmission_Target::ALTERNATIVE),
                    0b0001'1000);

  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::ANALOG_INPUT,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE),
                    0b0000'0000);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE),
                    0b0000'1010);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_SMALL_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE),
                    0b0001'0100);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_BIG_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE),
                    0b0000'1011);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::NEG_SMALL_REF,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE),
                    0b0001'0101);
  TEST_ASSERT_EQUAL(ublock.change_transmission_mode(UBlock::Transmission_Mode::GROUND,
                                                    UBlock::Transmission_Target::REGULAR_AND_ALTERNATIVE),
                    0b0001'1110);
}

int main(int argc, char **argv) {
  // The native platform does not use setup/loop functions, but a single main() function
  // When using "debug_test = native/examples/test_debugging" in platformio.ini, the debugger will start here.
  UNITY_BEGIN();
  RUN_TEST(test_a_test);
  UNITY_END();
}
