// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public
#include "carrier/carrier.h"

using namespace carrier;

Carrier _carrier_board;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() { TEST_ASSERT(_carrier_board.init()); }

void test_entity_id() { TEST_ASSERT_EQUAL_STRING("04-E9-E5-14-74-BF", _carrier_board.entity_id.c_str()); }

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_entity_id);
  UNITY_END();
}

void loop() { delay(100); }
