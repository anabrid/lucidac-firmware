// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define protected public
#define private public
#include "block.h"
#include "cblock.h"
#include "functions.h"

blocks::CScaleSwitchFunction switcher{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER)};
functions::TriggerFunction switcher_sync{bus::idx_to_addr(0,bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_SYNC)};
functions::TriggerFunction switcher_clear{bus::idx_to_addr(0,bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_CLEAR)};

blocks::CBlock cblock{0};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Put start-up sequence into a test case, so we can assert it worked.
  bus::init();
  TEST_ASSERT(cblock.init());
}

void test_address() {
  TEST_ASSERT_EQUAL(0b100001'0010, switcher.address);
}

void test_function() {
  switcher.data = 0b10001010'10001010'11111111'10001010;
  //switcher.data = 0b11111111'11111111'11111111'11111111;
  switcher.write_to_hardware();
  switcher_sync.trigger();

  //delayMicroseconds(1);
  //switcher_clear.trigger();
  //switcher_sync.trigger();
}

void test_via_block() {
  cblock.set_upscaling(0, true);
  cblock.set_upscaling(7, true);
  cblock.set_upscaling(27, true);
  TEST_ASSERT_EQUAL(0b00001000'00000000'00000000'10000001, cblock.upscaling_);
  cblock.write_upscaling_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_address);
  RUN_TEST(test_function);
  RUN_TEST(test_via_block);
  UNITY_END();
}

void loop() {}