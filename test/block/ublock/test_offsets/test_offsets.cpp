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

#include "ublock.h"

using namespace blocks;
using namespace blocks::functions;

UBlock uBlock{0};
UOffsetLoader offset_loader { bus::idx_to_addr(0, UBlock::BLOCK_IDX, 0) };

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
  uBlock.connect(7,0);
  uBlock.connect(7,1);
  uBlock.connect(7,2);
  uBlock.write_to_hardware();

  offset_loader.set_offset_and_write_to_hardware(0,0);
  offset_loader.set_offset_and_write_to_hardware(1,511);
  offset_loader.set_offset_and_write_to_hardware(2,1023);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_cmd_word);
  //RUN_TEST(test_load_trigger);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}