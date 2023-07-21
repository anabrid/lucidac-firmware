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
#include "mblock.h"
#include "ublock.h"
#include "cblock.h"
#include "iblock.h"

using namespace blocks;

MIntBlock intblock1{0, MBlock::M1_IDX};
MIntBlock intblock2{0, MBlock::M2_IDX};
UBlock ublock{0};
CBlock cblock{0};
IBlock iblock{0};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_entity_ids() {
  TEST_ASSERT_EQUAL_STRING("M0", intblock1.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("M1", intblock2.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("U", ublock.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("C", cblock.get_entity_id().c_str());
  TEST_ASSERT_EQUAL_STRING("I", iblock.get_entity_id().c_str());
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_entity_ids);
  UNITY_END();
}

void loop() {
  //test_function();
  delay(100);
}
