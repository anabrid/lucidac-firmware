// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "io/io.h"

#define private public
#define protected public
#include "block/cblock.h"
#include "block/ublock.h"

using namespace blocks;

CBlock *cblock;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  TEST_ASSERT(true);
  cblock = entities::detect<CBlock>(bus::C_BLOCK_BADDR(0));
  TEST_ASSERT_NOT_NULL(cblock);
  TEST_ASSERT(cblock->init());

  // We need signals from the U-block to see anything
  auto ublock = entities::detect<UBlock>(bus::U_BLOCK_BADDR(0));
  TEST_ASSERT_NOT_NULL(ublock);
  TEST_ASSERT(ublock->init());
  for (auto output_idx : UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(ublock->connect(output_idx % 16, output_idx));
  }
  TEST_ASSERT(ublock->write_to_hardware());
  ublock->change_all_transmission_modes(UBlock::Transmission_Mode::POS_REF);
  ublock->change_reference_magnitude(UBlock::Reference_Magnitude::ONE);
  TEST_ASSERT(ublock->write_to_hardware());
}

void test_factors() {
  for (auto idx : CBlock::OUTPUT_IDX_RANGE())
    // This outputs +-1.xx volts, where xx=idx
    TEST_ASSERT(cblock->set_factor(idx, (idx % 2 ? -0.5f : 0.5f) * (1.0f + static_cast<float>(idx) / 100.0f)));
  TEST_ASSERT(cblock->write_to_hardware());
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_factors);
  UNITY_END();
}

void loop() { delay(500); }
