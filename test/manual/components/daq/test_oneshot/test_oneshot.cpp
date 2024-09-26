// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#define protected public
#include "daq/daq.h"
#include "lucidac/lucidac.h"

using namespace daq;
OneshotDAQ DAQ{};
LUCIDAC lucidac;

void tearDown() {
  // clean stuff up here
}

void test_init() {
  TEST_ASSERT(lucidac.init());
  lucidac.reset(entities::ResetAction::EVERYTHING);
  TEST_ASSERT(lucidac.write_to_hardware());
  TEST_ASSERT(DAQ.init(0));
}

void test_raw_to_float() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, OneshotDAQ::raw_to_float(4094));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, +1.25f, OneshotDAQ::raw_to_float(OneshotDAQ::RAW_PLUS_ONE_POINT_TWO_FIVE));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.25f, OneshotDAQ::raw_to_float(OneshotDAQ::RAW_MINUS_ONE_POINT_TWO_FIVE));
  // Slightly outside of range
  TEST_ASSERT_FLOAT_WITHIN(0.01f, +1.2f,
                           OneshotDAQ::raw_to_float(OneshotDAQ::RAW_PLUS_ONE_POINT_TWO_FIVE -
                                                    0.1 * (OneshotDAQ::RAW_PLUS_ONE_POINT_TWO_FIVE -
                                                           OneshotDAQ::RAW_MINUS_ONE_POINT_TWO_FIVE)));
}

void test_sample_raw() {
  auto data_raw = DAQ.sample_avg_raw(40, 10); // We should always do average runs

  decltype(data_raw) expected{8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192};
  decltype(data_raw)::value_type acceptable_delta = 100;
  std::cout << "DAQ.sample_raw() = " << data_raw << std::endl;

  std::array<float, daq::NUM_CHANNELS> data{};
  std::transform(std::begin(data_raw), std::end(data_raw), std::begin(data), BaseDAQ::raw_to_float);
  std::cout << "Float values = " << data << std::endl;

  TEST_ASSERT_UINT16_ARRAY_WITHIN(acceptable_delta, expected.data(), data_raw.data(), data_raw.size());
}

void setup() {
  msg::activate_serial_log();
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_sample_raw);
  // RUN_TEST(test_raw_to_float);
  UNITY_END();
}

void loop() {}
