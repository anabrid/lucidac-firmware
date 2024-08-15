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
#include "mode/mode.h"

#define private public
#define protected public
#include "block/mblock.h"

using namespace blocks;

MIntBlock *mblock_int;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  // TODO: Do not assume address as fixed
  mblock_int = entities::detect<MIntBlock>(bus::idx_to_addr(0, bus::M0_BLOCK_IDX, 0));
  TEST_ASSERT_NOT_NULL(mblock_int);
  TEST_ASSERT(mblock_int->init());
}

void test_write_to_hardware() {
  TEST_ASSERT(mblock_int->set_ic_values({0.50f, -0.51f, 0.52f, -0.53f, 0.54f, -0.55f, 0.56f, -0.57f}));
  TEST_ASSERT(mblock_int->set_time_factors({100, 10000, 10000, 100, 10000, 100, 10000, 10000}));
  TEST_ASSERT(mblock_int->write_to_hardware());
}

void setup() {
  bus::init();
  io::init();
  // Some things can only be measured when IC is active
  mode::ManualControl::init();
  mode::ManualControl::to_ic();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_write_to_hardware);
  UNITY_END();
}

void loop() { delay(500); }
