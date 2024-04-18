// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
#include "daq/daq.h"

using namespace lucidac;
using namespace daq;

LUCIDAC luci{};
OneshotDAQ daq_{};

void setUp() {
  // set stuff up here
  luci.init();
  daq_.init(0);
}

void tearDown() {
  // clean stuff up here
}

void LOG_DATA_VECTOR(data_vec_t& data) {
  for (unsigned int i_ = 0; i_ < data.size(); i_++) {
    char buffer[4 + 33] = {' ', ' ', '=', ' '};
    buffer[0] = '0' + i_;
    sprintf(buffer + 4, "%.4f", data[i_]);
    TEST_MESSAGE(buffer);
  }
  TEST_MESSAGE("");
}

void test_lucidac() {
  TEST_ASSERT_EQUAL(0b00000'0011, luci.ublock->get_block_address());
}

void test_calibrate() {
  auto data_before = daq_.sample_avg(10, 10000);
  LOG_DATA_VECTOR(data_before);

  TEST_ASSERT_MESSAGE(luci.calibrate(&daq_), "Calibration failed.");

  auto data_after = daq_.sample_avg(10, 10000);
  LOG_DATA_VECTOR(data_after);

  data_vec_t data_should{};
  std::fill(std::begin(data_should), std::end(data_should), -1.0f);
  TEST_ASSERT_FLOAT_ARRAY_WITHIN(0.01f, data_should.data(), data_after.data(), data_after.size());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_lucidac);
  RUN_TEST(test_calibrate);
  UNITY_END();
}

void loop() {}
