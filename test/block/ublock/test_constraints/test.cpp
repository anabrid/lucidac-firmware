// Copyright (c) 2022 anabrid GmbH
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

#include "block/ublock.h"

using namespace blocks;

UBlock ublock{0};

void setUp() {
  // This is called before *each* test.
  ublock.reset(false);
}

void tearDown() {
  // This is called after *each* test.
}

void test_multiple_output_constraints() {
  // Connect one input to an output
  TEST_ASSERT(ublock.connect(0, 0));
  TEST_ASSERT(ublock.is_connected(0, 0));
  // Connect the same input to another output
  TEST_ASSERT(ublock.connect(0, 1));
  TEST_ASSERT(ublock.is_connected(0, 1));

  // Connecting another input to the same output would disconnect the previous ones
  // To reduce unintended errors, disconnecting other connections must explicitly be allowed
  // This connection will fail
  TEST_ASSERT_FALSE(ublock.connect(1, 0));
  TEST_ASSERT(ublock.is_connected(0, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
  // Allow disconnection to succeed
  TEST_ASSERT(ublock.connect(1, 0, true));
  TEST_ASSERT(ublock.is_connected(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(0, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));

  // Try to unsuccessfully overwrite again
  TEST_ASSERT_FALSE(ublock.connect(2, 0));
  TEST_ASSERT(ublock.is_connected(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(2, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
  // Clean up by disconnecting first
  TEST_ASSERT(ublock.disconnect(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(1, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(2, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
  // Then we can connect without any disconnections
  TEST_ASSERT(ublock.connect(2, 0));
  TEST_ASSERT_FALSE(ublock.is_connected(1, 0));
  TEST_ASSERT(ublock.is_connected(2, 0));
  TEST_ASSERT(ublock.is_connected(0, 1));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_multiple_output_constraints);
  UNITY_END();
}

void loop() { delay(500); }
