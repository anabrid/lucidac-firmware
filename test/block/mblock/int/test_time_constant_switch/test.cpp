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
#include "block/mblock.h"

using namespace blocks;

MIntBlock intblock{0, MBlock::M1_IDX};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Initialize bus
  bus::init();
  // MIntBlock initializes with DEFAULT_TIME_FACTOR
  TEST_ASSERT_EACH_EQUAL_UINT32(MIntBlock::DEFAULT_TIME_FACTOR, intblock.time_factors.data(), intblock.time_factors.size());
}

void test_function() {
  intblock.f_time_factor.transfer(0b01011011);
  intblock.f_time_factor_sync.trigger();
  TEST_ASSERT(true);
}

void test_block() {
  // Invalid k
  TEST_ASSERT_FALSE(intblock.set_time_factor(0, 1));
  TEST_ASSERT_FALSE(intblock.set_time_factor(1, 123));
  TEST_ASSERT_FALSE(intblock.set_time_factor(0, 90000));
  TEST_ASSERT_FALSE(intblock.set_time_factor(1, 1000000));
  // Invalid idx
  TEST_ASSERT_FALSE(intblock.set_time_factor(8, 100));
  TEST_ASSERT_FALSE(intblock.set_time_factor(13, 10000));

  // Valid commands
  TEST_ASSERT(intblock.set_time_factor(0, 100));
  TEST_ASSERT_EQUAL(100, intblock.time_factors[0]);
  TEST_ASSERT(intblock.set_time_factor(1, 10000));
  TEST_ASSERT_EQUAL(10000, intblock.time_factors[1]);

  // Write to hardware
  intblock.write_time_factors_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  //RUN_TEST(test_function);
  RUN_TEST(test_block);
  UNITY_END();
}

void loop() {
  //test_function();
  delay(100);
}
