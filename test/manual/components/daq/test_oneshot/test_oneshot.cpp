// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define protected public
#include "daq/daq.h"

using namespace daq;
OneshotDAQ DAQ{};

void setUp() {
  // set stuff up here
  DAQ.init(0);
}

void tearDown() {
  // clean stuff up here
}

void test_raw_to_float() {
  TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, OneshotDAQ::raw_to_float(4094));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, +1.0f, OneshotDAQ::raw_to_float(OneshotDAQ::RAW_PLUS_ONE));
  TEST_ASSERT_FLOAT_WITHIN(0.01f, -1.0f, OneshotDAQ::raw_to_float(OneshotDAQ::RAW_MINUS_ONE));
  // Slightly outside of range
  TEST_ASSERT_FLOAT_WITHIN(
      0.01f, +1.2f,
      OneshotDAQ::raw_to_float(OneshotDAQ::RAW_PLUS_ONE +
                               0.1 * (OneshotDAQ::RAW_PLUS_ONE - OneshotDAQ::RAW_MINUS_ONE)));
}

void test_sample() {
  auto data = DAQ.sample();
  for (unsigned int i_ = 0; i_ < data.size(); i_++) {
    char buffer[4 + 33] = {' ', ' ', '=', ' '};
    buffer[0] = '0' + i_;
    sprintf(buffer + 4, "%.4f", data[i_]);
    TEST_MESSAGE(buffer);
  }
}

void test_sample_raw() {
  auto data = DAQ.sample_raw();
  TEST_ASSERT_EQUAL(daq::NUM_CHANNELS, data.size());

  // Without other blocks, we should be at a certain value.
  // Not sure which one, but surely not zero :)
  // TODO: After ADCs are fixed, add correct values.
  decltype(data) expected{0, 0, 0, 0 /*, 0, 0, 0, 0*/};
  decltype(data)::value_type acceptable_delta = 1000;
  for (unsigned int i_ = 0; i_ < data.size(); i_++) {
    char buffer[4 + 33] = {' ', ' ', '=', ' '};
    buffer[0] = '0' + i_;
    itoa(data[i_], buffer + 4, 2);
    TEST_MESSAGE(buffer);
    itoa(data[i_], buffer + 4, 10);
    TEST_MESSAGE(buffer);
  }
  TEST_ASSERT_UINT16_ARRAY_WITHIN(acceptable_delta, expected.data(), data.data(), data.size());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_sample);
  RUN_TEST(test_sample_raw);
  RUN_TEST(test_raw_to_float);
  UNITY_END();
}

void loop() {}
