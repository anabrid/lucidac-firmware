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
