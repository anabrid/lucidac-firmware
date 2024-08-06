// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "block/blocks.h"

using namespace blocks;

blocks::IBlock *iblock;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  iblock = entities::detect<blocks::IBlock>(bus::idx_to_addr(0, bus::I_BLOCK_IDX, 0));
  TEST_ASSERT_NOT_NULL(iblock);
  TEST_ASSERT(iblock->init());
}

void test_scaling_register() {
  std::bitset<IBlock::NUM_INPUTS> factors;

  factors.set();
  iblock->set_upscaling(factors);
  TEST_ASSERT(iblock->hardware->write_upscaling(iblock->get_upscales()));

  delay(10);
  factors.reset();
  iblock->set_upscaling(factors);
  TEST_ASSERT(iblock->hardware->write_upscaling(iblock->get_upscales()));

  delay(10);
  factors = 0xCAFE;
  iblock->set_upscaling(factors);
  TEST_ASSERT(iblock->hardware->write_upscaling(iblock->get_upscales()));

  delay(10);
  iblock->set_upscaling(31, true);
  iblock->set_upscaling(30, true);
  TEST_ASSERT(iblock->hardware->write_upscaling(iblock->get_upscales()));
}

void test_matrix() {
  for (auto output_idx : IBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(iblock->connect(2 * output_idx, output_idx));
    TEST_ASSERT(iblock->connect(2 * output_idx + 1, output_idx));
  }
  TEST_ASSERT(iblock->hardware->write_outputs(iblock->get_outputs()));
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_scaling_register);
  RUN_TEST(test_matrix);
  UNITY_END();
}

void loop() { delay(500); }
