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