// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#include "block/cblock.h"
#include "block/iblock.h"
#include "block/mblock.h"
#include "block/ublock.h"

using namespace blocks;

MIntBlock intblock1{0, MBlock::M1_IDX};
MIntBlock intblock2{0, MBlock::M2_IDX};
UBlock ublock{0};
CBlock cblock{0};
IBlock iblock{0};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_entity_ids() {
  TEST_ASSERT_EQUAL_STRING("M0", intblock1.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("M1", intblock2.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("U", ublock.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("C", cblock.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("I", iblock.get_entity_id().c_str());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_entity_ids);
  UNITY_END();
}

void loop() {
  //test_function();
  delay(100);
}
