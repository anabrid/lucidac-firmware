// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;

LUCIDAC lucidac;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() { TEST_ASSERT(lucidac.init()); }

void test_acl() {
  TEST_ASSERT_FALSE(lucidac.set_acl_select(8, LUCIDAC::ACL::EXTERNAL_));
  lucidac.set_acl_select({LUCIDAC::ACL::INTERNAL_, LUCIDAC::ACL::INTERNAL_, LUCIDAC::ACL::EXTERNAL_,
                          LUCIDAC::ACL::INTERNAL_, LUCIDAC::ACL::INTERNAL_, LUCIDAC::ACL::EXTERNAL_,
                          LUCIDAC::ACL::INTERNAL_, LUCIDAC::ACL::EXTERNAL_});
  TEST_ASSERT_EQUAL(LUCIDAC::ACL::INTERNAL_, lucidac.get_acl_select()[0]);
  TEST_ASSERT(lucidac.set_acl_select(0, LUCIDAC::ACL::EXTERNAL_));
  TEST_ASSERT_EQUAL(LUCIDAC::ACL::EXTERNAL_, lucidac.get_acl_select()[0]);
  TEST_ASSERT(lucidac.write_to_hardware());
}

void test_adc_channels() {
  TEST_ASSERT_FALSE(lucidac.set_adc_channels({0, 1, 2, 3, 4, 5, 6, 32}));
  TEST_ASSERT_FALSE(lucidac.set_adc_channels({0, 0, 0, 0, 0, 0, 0, 0}));
  TEST_ASSERT(lucidac.set_adc_channels(
      {LUCIDAC::ADC_CHANNEL_DISABLED, 1, 2, 3, LUCIDAC::ADC_CHANNEL_DISABLED, 5, 6, 7}));

  // Do not use a channel twice
  TEST_ASSERT_FALSE(lucidac.set_adc_channel(0, 1));
  TEST_ASSERT_EQUAL(LUCIDAC::ADC_CHANNEL_DISABLED, lucidac.get_adc_channels()[0]);
  TEST_ASSERT(lucidac.set_adc_channel(0, LUCIDAC::ADC_CHANNEL_DISABLED));
  TEST_ASSERT(lucidac.set_adc_channel(7, 15));

  TEST_ASSERT(lucidac.write_to_hardware());
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_acl);
  RUN_TEST(test_adc_channels);
  UNITY_END();
}

void loop() { delay(500); }
