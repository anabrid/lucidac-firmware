// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "mode/mode.h"

using namespace mode;

void setUp() {
  // This is called before *each* test.
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
}

void tearDown() {
  // This is called after *each* test.
  digitalWriteFast(LED_BUILTIN, LOW);
}

void test_limits() {
  // Too short
  TEST_ASSERT_FALSE(FlexIOControl::init(99, mode::DEFAULT_OP_TIME));
  TEST_ASSERT_FALSE(FlexIOControl::init(mode::DEFAULT_IC_TIME, 99));
  // Too long
  TEST_ASSERT_FALSE(FlexIOControl::init(275'000, mode::DEFAULT_OP_TIME));
  TEST_ASSERT_FALSE(FlexIOControl::init(mode::DEFAULT_IC_TIME, 9'000'000'000));
}

void test_simple_run() {
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME));
  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
  }
}

void test_approximate_run_time() {
  for (auto op_time_ns : {1'000'000ull, 1'000ull, 32 * 10'000ull}) {
    TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, op_time_ns));

    auto t_start = micros();
    FlexIOControl::force_start();
    while (!FlexIOControl::is_done()) {
    }
    auto t_end = micros();

    if (t_end < t_start)
      TEST_FAIL_MESSAGE("micros timer overflow, just rerun :)");
    TEST_ASSERT_UINT_WITHIN(10, (mode::DEFAULT_IC_TIME + op_time_ns) / 1000, t_end - t_start);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_limits);
  RUN_TEST(test_simple_run);
  RUN_TEST(test_approximate_run_time);
  UNITY_END();
}

void loop() {
  /*
  static auto op_time = mode::DEFAULT_OP_TIME;
  delay(max(op_time/1'000'000, 10000));
  if (FlexIOControl::init(mode::DEFAULT_IC_TIME, op_time)) {
    FlexIOControl::force_start();
    op_time *= 2;
  } else {
    op_time = mode::DEFAULT_OP_TIME;
  }
  */
}
