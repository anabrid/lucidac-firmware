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
#include "block/mblock.h"

using namespace blocks;

MMulBlock *mblock_mul;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  // TODO: Do not assume address as fixed
  mblock_mul = entities::detect<MMulBlock>(bus::idx_to_addr(0, bus::M1_BLOCK_IDX, 0));
  TEST_ASSERT_NOT_NULL(mblock_mul);
  TEST_ASSERT(mblock_mul->init());
}

void test_write_to_hardware() {
  TEST_ASSERT(mblock_mul->hardware->write_calibration_input_offsets(0, +0.05, -0.05));
  TEST_ASSERT(mblock_mul->hardware->write_calibration_output_offset(0, +0.05));
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_write_to_hardware);
  UNITY_END();
}

void loop() { delay(500); }
