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

Carrier carrier_board({});

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() { TEST_ASSERT(carrier_board.init()); }

void test_acl_prg() {
  carrier_board.f_acl_prg.transfer8(0b11100110);
  carrier_board.f_acl_upd.trigger();
  TEST_ASSERT(true);
}

void test_acl_clear() {
  carrier_board.f_acl_clr.trigger();
  carrier_board.f_acl_upd.trigger();
  TEST_ASSERT(true);
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  // RUN_TEST(test_init);
  RUN_TEST(test_acl_prg);
  // RUN_TEST(test_acl_clear);
  UNITY_END();
}

void loop() { delay(100); }
