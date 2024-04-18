// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
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
// for further ublock_signal_switcher.cppagreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "block/ublock.h"
#include "bus/bus.h"

using namespace blocks;

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_function() {
  UBlock ublock{0};
  TEST_ASSERT_EQUAL(0, ublock.get_alt_signals());

  TEST_ASSERT(ublock.use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));
  TEST_ASSERT_EQUAL(UBlock::ALT_SIGNAL_REF_HALF, ublock.get_alt_signals());

  TEST_ASSERT_FALSE(ublock.use_alt_signals(UBlock::MAX_ALT_SIGNAL + 1));

  TEST_ASSERT(ublock.use_alt_signals(UBlock::ALT_SIGNAL_ACL7));
  TEST_ASSERT_EQUAL(UBlock::ALT_SIGNAL_REF_HALF | UBlock::ALT_SIGNAL_ACL7, ublock.get_alt_signals());

  ublock.write_alt_signal_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
