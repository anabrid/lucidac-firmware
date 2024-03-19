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

#include "bus/bus.h"
#include "chips/TMP127Q1.h"

using namespace functions;

// New REV1 addresses
// This one is on C-Block
//TMP127Q1 chip{bus::address_from_tuple(9, 33)};
// This one is on carrier board
TMP127Q1 chip{bus::address_from_tuple(5, 1)};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_raw_conversion() {
  // Assert that right-shift of signed values is an arithmetic shift,
  // which apparently is compiler and processor specific.
  TEST_ASSERT_EQUAL(-800, TMP127Q1::raw_to_signed_raw(0b1111'0011'1000'0011));
}

void test_chip_function() {
  TEST_ASSERT_FLOAT_WITHIN(10, 30, chip.read_temperature());
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  RUN_TEST(test_raw_conversion);
  RUN_TEST(test_chip_function);
  UNITY_END();
}

void loop() {
  pinMode(29, INPUT_PULLUP);

  // Do an action once the button is pressed
  while (digitalReadFast(29)) {
  }
  test_chip_function();
  delay(500);
}