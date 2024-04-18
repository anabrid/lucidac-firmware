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

Carrier carrier_board;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  TEST_ASSERT(carrier_board.init());
}

void test_acl_prg() {
  bus::address_function(bus::address_from_tuple(Carrier::CARRIER_MADDR, Carrier::ACL_PRG_FADDR));
  bus::activate_address();
  //carrier_board.f_acl_prg.transfer(0b11001111);
  TEST_ASSERT(true);
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  //RUN_TEST(test_init);
  RUN_TEST(test_acl_prg);
  UNITY_END();
}

void loop() { delay(100); }
