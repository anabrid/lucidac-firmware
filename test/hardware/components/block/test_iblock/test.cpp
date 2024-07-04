// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define ANABRID_PEDANTIC

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "block/blocks.h"

using namespace blocks;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
blocks::IBlock *iblock;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  iblock = blocks::detect<blocks::IBlock>(bus::idx_to_addr(0, bus::I_BLOCK_IDX, 0));
  TEST_ASSERT_NOT_NULL(iblock);
}

void test_scaling_register() {
  std::bitset<IBlock::NUM_INPUTS> factors;

  factors.set();
  iblock->set_scales(factors);

  TEST_ASSERT(iblock->write_scaling_to_hardware());

  delay(10);

  factors.reset();
  iblock->set_scales(factors);

  TEST_ASSERT(iblock->write_scaling_to_hardware());

  delay(10);

  factors = 0xCAFE;
  iblock->set_scales(factors);

  TEST_ASSERT(iblock->write_scaling_to_hardware());
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_scaling_register);
  UNITY_END();
}

void loop() { delay(500); }
