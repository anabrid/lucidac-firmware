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

#include "iblock.h"

using namespace blocks;

IBlock iblock{0};

void setUp() {
  // This is called before *each* test.
  iblock.reset(false);
}

void tearDown() {
  // This is called after *each* test.
}

void test_split_signal_constraint() {
  // Connect one input to an output
  TEST_ASSERT(iblock.connect(0,0));
  TEST_ASSERT(iblock.is_connected(0,0));
  // And add another input, which is fine
  TEST_ASSERT(iblock.connect(1,0));
  TEST_ASSERT(iblock.is_connected(1,0));

  // But it's usually not wanted to split inputs to multiple signals
  TEST_ASSERT_FALSE(iblock.connect(1,1));
  TEST_ASSERT(iblock.is_connected(0,0));
  TEST_ASSERT(iblock.is_connected(1,0));
  TEST_ASSERT_FALSE(iblock.is_connected(1,1));
  // Maybe for some future applications (oscillator coupling), it may be useful.
  // Use allow_input_splitting to allow connecting one input to multiple outputs.
  TEST_ASSERT(iblock.connect(1,1, false,true));
  TEST_ASSERT(iblock.is_connected(0,0));
  TEST_ASSERT(iblock.is_connected(1,0));
  TEST_ASSERT(iblock.is_connected(1,1));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_split_signal_constraint);
  UNITY_END();
}

void loop() { delay(500); }
