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

#define protected public
#include "daq.h"

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

void test_sample_raw() {
  auto data = DAQ.sample_raw();
  TEST_ASSERT_EQUAL(daq::NUM_CHANNELS, data.size());

  // Without other blocks, we should be at a certain value.
  // Not sure which one, but surely not zero :)
  // TODO: After ADCs are fixed, add correct values.
  decltype(data) expected{0, 0, 0, 0, 0, 0, 0, 0};
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
  RUN_TEST(test_sample_raw);
  RUN_TEST(test_raw_to_float);
  UNITY_END();
}

void loop() {}