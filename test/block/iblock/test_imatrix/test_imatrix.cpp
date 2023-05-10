// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "iblock.h"
#include "local_bus.h"

using namespace bus;
using namespace blocks;
using namespace blocks::functions;

TriggerFunction imatrix_reset{idx_to_addr(0, IBlock::BLOCK_IDX, IBlock::IMATRIX_RESET_FUNC_IDX)};
TriggerFunction imatrix_cmd_sr_rest{
    idx_to_addr(0, IBlock::BLOCK_IDX, IBlock::IMATRIX_COMMAND_SR_RESET_FUNC_IDX)};
TriggerFunction imatrix_sync{idx_to_addr(0, IBlock::BLOCK_IDX, IBlock::IMATRIX_SYNC_FUNC_IDX)};

ICommandRegisterFunction imatrix_cmd_sr{
    idx_to_addr(0, IBlock::BLOCK_IDX, IBlock::IMATRIX_COMMAND_SR_FUNC_IDX)};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
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

// Currently skipped
void test_function() {
  // Reset signals arrive at chips
  imatrix_reset.trigger();
  delayMicroseconds(1);
  // imatrix_cmd_sr_rest.trigger();
  // delayMicroseconds(1);

  // imatrix_cmd_sr.data = 0b10000000'11111111'00000000'00000001;
  // imatrix_cmd_sr.data = 0b01111111'11111111'11111111'11111110;

  // Set I1 Matrix input 0 to output 0
  // This should connect BL_OUT.0 to MBL_IN.0
  imatrix_cmd_sr.data = 0b0'000'0000'0'000'0000'0'000'0000'1'000'0000;
  imatrix_cmd_sr.write_to_hardware();

  delayMicroseconds(1);
  imatrix_sync.trigger();
}

void test_block() {
  IBlock iblock{0};
  TEST_ASSERT_EACH_EQUAL_UINT32_MESSAGE(0, iblock.outputs.data(), iblock.outputs.size(),
                                        "IBlock outputs not initialized to zero.");

  iblock.outputs[0] = IBlock::INPUT_BITMASK(0);
  TEST_ASSERT_EQUAL(0b00000000'00000000'00000000'00000001, iblock.outputs[0]);
  TEST_ASSERT(iblock.outputs[0] & IBlock::INPUT_BITMASK(0));

  iblock.outputs[3] = IBlock::INPUT_BITMASK(1);
  TEST_ASSERT_EQUAL(0b00000000'00000000'00000000'00000010, iblock.outputs[3]);

  //iblock.outputs[3] = IBlock::INPUT_BITMASK(1) | IBlock::INPUT_BITMASK(7) | IBlock::INPUT_BITMASK(17) |
  //                    IBlock::INPUT_BITMASK(31);
  //TEST_ASSERT_EQUAL(0b10000000'00000010'00000000'10000010, iblock.outputs[3]);

  //iblock.outputs[15] = IBlock::INPUT_BITMASK(3) | IBlock::INPUT_BITMASK(20) | IBlock::INPUT_BITMASK(31);
  //TEST_ASSERT_EQUAL(0b10000000'00010000'00000000'00001000, iblock.outputs[15]);

  // This changes the SPI speed? lol
  //iblock.outputs[18] = IBlock::INPUT_BITMASK(3) | IBlock::INPUT_BITMASK(17);
  //TEST_ASSERT_EQUAL(0b00000000'00000010'00000000'00000010, iblock.outputs[18]);

  iblock.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_function_helpers);
  // RUN_TEST(test_function);
  RUN_TEST(test_block);
  UNITY_END();
}

void loop() {}
