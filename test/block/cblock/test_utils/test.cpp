// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#include "cblock.h"

using namespace functions;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_raw_to_float() {
  TEST_ASSERT_FLOAT_WITHIN(0.001, 0.0f, AD5452::raw_to_float(AD5452::RAW_ZERO << 2));
  TEST_ASSERT_FLOAT_WITHIN(0.001, -2.0f, AD5452::raw_to_float(0));
  TEST_ASSERT_FLOAT_WITHIN(0.001, +2.0f, AD5452::raw_to_float(4095<<2));
}

void test_float_to_raw() { TEST_ASSERT(true); }

void test_identity() {
  for (unsigned int i = -200; i <= +200; i++) {
    auto value = static_cast<float>(i) / 100.0f;
    TEST_ASSERT_FLOAT_WITHIN(0.001, value, AD5452::raw_to_float(AD5452::float_to_raw(value)));
  }
  for (unsigned int i = 0; i <= 4095; i++) {
    TEST_ASSERT_UINT_WITHIN(5 << 2, i, AD5452::float_to_raw(AD5452::raw_to_float(i << 2)));
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_raw_to_float);
  RUN_TEST(test_float_to_raw);
  RUN_TEST(test_identity);
  UNITY_END();
}

void loop() {
  // test_function();
  delay(100);
}
