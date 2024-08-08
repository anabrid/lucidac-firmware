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
#include "block/ublock.h"

using namespace blocks;

UBlock *ublock;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  ublock = entities::detect<UBlock>(bus::U_BLOCK_BADDR(0));
  TEST_ASSERT_NOT_NULL(ublock);
  TEST_ASSERT(ublock->init());
}

void test_matrix() {
  for (auto output_idx : UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(ublock->connect(output_idx % 16, output_idx));
  }
  TEST_ASSERT(ublock->write_to_hardware());
}

void test_transmission_modes_and_ref() {
  ublock->change_a_side_transmission_mode(UBlock::Transmission_Mode::POS_REF);
  ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::NEG_REF);
  ublock->change_reference_magnitude(UBlock::Reference_Magnitude::ONE_TENTH);
  TEST_ASSERT(ublock->write_to_hardware());
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_matrix);
  RUN_TEST(test_transmission_modes_and_ref);
  UNITY_END();
}

void loop() { delay(500); }
