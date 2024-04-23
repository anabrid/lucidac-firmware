// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "block/ublock.h"

using namespace functions;
using namespace blocks;

UBlock uBlock{0};
UOffsetLoader offset_loader{bus::idx_to_addr(0, UBlock::BLOCK_IDX, 0), UBlock::OFFSETS_DATA_FUNC_IDX,
                            UBlock::OFFSETS_LOAD_BASE_FUNC_IDX};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_cmd_word() {
  TEST_ASSERT_EQUAL(0b00'1000'0000000000, UOffsetLoader::build_cmd_word(0, 0));
  TEST_ASSERT_EQUAL(0b00'1000'1111111111, UOffsetLoader::build_cmd_word(0, 1023));
  TEST_ASSERT_EQUAL(0b00'1000'1111111111, UOffsetLoader::build_cmd_word(0, 1023 + 4096));
  TEST_ASSERT_EQUAL(0b00'1000'1111111111, UOffsetLoader::build_cmd_word(0, 1024));
  TEST_ASSERT_EQUAL(0b00'1000'0000000000 + 1023, UOffsetLoader::build_cmd_word(0, 1023));

  TEST_ASSERT_EQUAL(0b00'0010'0000000000, UOffsetLoader::build_cmd_word(3, 0));
  TEST_ASSERT_EQUAL(0b00'0001'0000000000, UOffsetLoader::build_cmd_word(7, 0));

  TEST_ASSERT_EQUAL(0b00'1010'0001111011, UOffsetLoader::build_cmd_word(4, 123));
  TEST_ASSERT_EQUAL(0b00'1110'1011110000, UOffsetLoader::build_cmd_word(6, 752));
}

void test_load_trigger() {
  for (auto i = 0; i < UBlock::NUM_OF_OUTPUTS; i++) {
    offset_loader.trigger_load(i);
  }
}

void test_function() {
  uBlock.use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF);
  uBlock.connect(7, 0);
  uBlock.connect(7, 1);
  uBlock.connect(7, 2);
  uBlock.write_to_hardware();

  offset_loader.set_offset_and_write_to_hardware(0, 0);
  offset_loader.set_offset_and_write_to_hardware(1, 511);
  offset_loader.set_offset_and_write_to_hardware(2, 1023);
}

void test_offset_from_block() {
  uBlock.reset_offsets();
  uBlock.use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF);
  uBlock.connect(7, 3);
  uBlock.connect(7, 4);
  uBlock.connect(7, 5);
  uBlock.write_to_hardware();

  TEST_ASSERT(uBlock.set_offset(3, 0.0f));
  // TEST_ASSERT(uBlock.set_offset(3, -0.010f/lucidac::REF_VOLTAGE));
  // TEST_ASSERT(uBlock.set_offset(3, +0.0481f/lucidac::REF_VOLTAGE));
  uBlock.write_offsets_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_cmd_word);
  RUN_TEST(test_function);
  RUN_TEST(test_offset_from_block);
  UNITY_END();
}

void loop() {}
