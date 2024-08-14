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
  decltype(data) expected{8192, 8192, 8192, 8192, 8192, 8192, 8192, 8192};
  decltype(data)::value_type acceptable_delta = 1000;
  std::cout << "DAQ.sample_raw() = ";
  for (auto &d : data)
    std::cout << d << ", ";
  std::cout << std::endl;
  TEST_ASSERT_UINT16_ARRAY_WITHIN(acceptable_delta, expected.data(), data.data(), data.size());
}

void setup() {
  UNITY_BEGIN();

  bus::init();
  TEST_ASSERT(lucidac.init());
  TEST_ASSERT(DAQ.init(0));

  // important, because sets Carrier::reset_adc_channels and CtrlBlock::reset_adc_bus,
  // thus we read from ADCBus not Gain and we read from the *empty* MT8816 adc_channel matrix
  lucidac.reset(false);

  // RUN_TEST(test_sample);
  RUN_TEST(test_sample_raw);
  // RUN_TEST(test_raw_to_float);
  UNITY_END();
}

void loop() {}
