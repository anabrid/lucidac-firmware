// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;
using ACL = LUCIDAC_HAL::ACL;

LUCIDAC_HAL hal;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_acl_prg() {
  TEST_ASSERT(hal.write_acl({ACL::EXTERNAL_, ACL::EXTERNAL_, ACL::INTERNAL_, ACL::EXTERNAL_, ACL::INTERNAL_,
                             ACL::INTERNAL_, ACL::EXTERNAL_, ACL::INTERNAL_}));
}

void test_acl_clear() {
  hal.reset_acl();
  TEST_ASSERT(true);
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(test_acl_prg);
  io::block_until_button_press();
  RUN_TEST(test_acl_clear);
  UNITY_END();
}

void loop() { delay(100); }
