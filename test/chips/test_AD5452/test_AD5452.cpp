// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define protected public
#include "functions.h"
#include "AD5452.h"

using namespace functions;

auto addr = bus::idx_to_addr(0, bus::C_BLOCK_IDX, 1);
AD5452 mdac{addr};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_float_to_raw() {
  TEST_ASSERT_EQUAL(0, AD5452::float_to_raw(-1.0f));
  TEST_ASSERT_EQUAL(0, AD5452::float_to_raw(-4.2f));

  TEST_ASSERT_EQUAL(2047 << 2, AD5452::float_to_raw(0.0f));

  TEST_ASSERT_EQUAL(4095 << 2, AD5452::float_to_raw(+1.0f));
  TEST_ASSERT_EQUAL(4095 << 2, AD5452::float_to_raw(+4.2f));

  TEST_ASSERT_UINT16_WITHIN(5, (1*1024) << 2, AD5452::float_to_raw(-0.5f));
  TEST_ASSERT_UINT16_WITHIN(5, (3*1024) << 2, AD5452::float_to_raw(+0.5f));
}

void test_set_scale_raw() {
  mdac.set_scale(static_cast<uint16_t>(4095<<2));
  TEST_ASSERT(true);
}

void test_set_scale() {
  mdac.set_scale(-0.25f);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_float_to_raw);
  RUN_TEST(test_set_scale_raw);
  RUN_TEST(test_set_scale);
  UNITY_END();
}

void loop() {
}