// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifdef ANABRID_PEDANTIC
#error "Emulated test cases expect pedantic mode to be disabled."
#endif

#include "bus/bus.h"
#include "bus/functions.h"

#define private public
#define protected public
#include "block/iblock.h"

using namespace blocks;
using namespace bus;
using namespace functions;
using namespace fakeit;

TriggerFunction imatrix_reset{bus::address_from_tuple(10, 4)};
TriggerFunction imatrix_sync{bus::address_from_tuple(10, 3)};

ICommandRegisterFunction imatrix_cmd_sr{bus::address_from_tuple(10, 2)};

void setUp() {
  // This is called before *each* test.
  ArduinoFakeReset();

  // Mock bus::init calls
  When(Method(ArduinoFake(SPI), begin)).AlwaysReturn();
  When(Method(ArduinoFake(Function), pinMode)).AlwaysReturn();
  When(Method(ArduinoFake(Function), digitalWrite)).AlwaysReturn();

  // Mock all delay* calls
  When(Method(ArduinoFake(Function), delay)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayMicroseconds)).AlwaysReturn();
  When(Method(ArduinoFake(Function), delayNanoseconds)).AlwaysReturn();

  // Mock GPIO calls
  When(OverloadedMethod(ArduinoFake(Function), pinMode, void(uint8_t, uint8_t))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(Function), digitalWrite, void(uint8_t, uint8_t))).AlwaysReturn();
  When(Method(ArduinoFake(Function), digitalRead)).AlwaysReturn(0);
}

void tearDown() {
  // This is called after *each* test.
}

void test_function_helpers() {
  // Test construction of one 8bit command word for one chip
  const auto chip_cmd_word_ = ICommandRegisterFunction::chip_cmd_word;
  TEST_ASSERT_EQUAL(0b0'000'0000, chip_cmd_word_(0, 0, false));
  TEST_ASSERT_EQUAL(0b1'000'0000, chip_cmd_word_(0, 0, true));
  TEST_ASSERT_EQUAL(0b1'000'0000, ICommandRegisterFunction::chip_cmd_word(0, 0));
  TEST_ASSERT_EQUAL(0b1'011'0000, chip_cmd_word_(0, 3, true));
  TEST_ASSERT_EQUAL(0b1'111'0000, chip_cmd_word_(0, 7, true));
  TEST_ASSERT_EQUAL(0b1'011'0001, chip_cmd_word_(1, 3, true));
  TEST_ASSERT_EQUAL(0b0'011'0001, chip_cmd_word_(1, 3, false));
  TEST_ASSERT_EQUAL(0b1'111'0100, chip_cmd_word_(4, 7, true));
  TEST_ASSERT_EQUAL(0b1'010'1101, chip_cmd_word_(13, 2, true));

  // Test combination of multiple 8bit commands into one 32bit command
  // This automatically does integer promotion (probably)
  uint32_t command = chip_cmd_word_(4, 7, true) << 24 | chip_cmd_word_(1, 3, false) << 8;
  command |= chip_cmd_word_(0, 0, true) << 16;
  command |= chip_cmd_word_(13, 2, true);
  TEST_ASSERT_EQUAL(0b1'111'0100'1'000'0000'0'011'0001'1'010'1101, command);
}

void test_block() {
  IBlock iblock(bus::NULL_ADDRESS, new IBlockHAL_V_1_2_0(bus::NULL_ADDRESS));
  TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, iblock.outputs.data(), iblock.outputs.size(),
                                        "IBlock outputs not initialized to zero.");

  iblock.outputs[0] = IBlock::INPUT_BITMASK(0);
  TEST_ASSERT_EQUAL(0b00000000'00000000'00000000'00000001, iblock.outputs[0]);
  TEST_ASSERT(iblock.outputs[0] & IBlock::INPUT_BITMASK(0));

  iblock.outputs[3] = IBlock::INPUT_BITMASK(1);
  TEST_ASSERT_EQUAL(0b00000000'00000000'00000000'00000010, iblock.outputs[3]);

  // Mock SPI calls
  // Ignore transaction handling
  When(Method(ArduinoFake(SPI), beginTransaction)).AlwaysReturn();
  When(Method(ArduinoFake(SPI), endTransaction)).AlwaysReturn();
  // Ignore transfer16 calls, which here are used only for setting address and triggering things
  When(Method(ArduinoFake(SPI), transfer16)).AlwaysReturn();
  // Sending the data uses 2 x transfer32, and each time a sync is done via a TriggerFunction
  When(Method(ArduinoFake(SPI), transfer32)).Return(0, 0);
  // Do it!
  iblock.hardware->write_outputs(iblock.get_outputs());

  // verify SPI call arguments, which are interlaced by address setting
  Verify(Method(ArduinoFake(SPI), transfer32).Using(ICommandRegisterFunction::chip_cmd_word(0, 0)) +
         Method(ArduinoFake(SPI), endTransaction) + Method(ArduinoFake(SPI), beginTransaction) +
         Method(ArduinoFake(SPI), transfer16) + Method(ArduinoFake(SPI), endTransaction) +
         Method(ArduinoFake(SPI), beginTransaction) + Method(ArduinoFake(SPI), transfer16) +
         Method(ArduinoFake(SPI), endTransaction) + Method(ArduinoFake(SPI), beginTransaction) +
         Method(ArduinoFake(SPI), transfer32).Using(ICommandRegisterFunction::chip_cmd_word(1, 3)));
}

int main() {
  // Call setUp to set up mocking
  setUp();

  // Mock SPI configuration calls
  When(OverloadedMethod(ArduinoFake(SPI), begin, void(void))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(SPI), end, void(void))).AlwaysReturn();
  bus::init();

  UNITY_BEGIN();
  // RUN_TEST(test_function_helpers);
  RUN_TEST(test_block);
  UNITY_END();
}

void loop() {}
