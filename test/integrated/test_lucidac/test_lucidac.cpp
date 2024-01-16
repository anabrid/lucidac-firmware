// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
#include "daq.h"

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
