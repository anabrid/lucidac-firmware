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

#include "ublock.h"

using namespace blocks;

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_block() {
  UBlock ublock{0};
  // So we have at least one concrete input signal
  ublock.use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF);

  TEST_ASSERT(ublock.connect(7,0));
  TEST_ASSERT(ublock.connect(7,5));
  TEST_ASSERT(ublock.connect(7,10));
  TEST_ASSERT(ublock.connect(7,15));
  TEST_ASSERT(ublock.connect(7,20));
  TEST_ASSERT(ublock.connect(7,25));
  TEST_ASSERT(ublock.connect(7,30));
  ublock.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_block);
  UNITY_END();
}

void loop() {}
