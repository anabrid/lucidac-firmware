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

#include "block.h"

blocks::CCoeffFunction coeff{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::COEFF_BASE_FUNC_IDX), 0};
blocks::CCoeffFunction coeff_two{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::COEFF_BASE_FUNC_IDX),
                                 1};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_address() {
  // First function on C-Block is metadata storage, first coeff has function idx 1.
  TEST_ASSERT_EQUAL(0b000001'0010, coeff.address);
  TEST_ASSERT_EQUAL(0b000010'0010, coeff_two.address);
}

void test_function() {
  coeff.data = 0x3FFF;
  coeff.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_address);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
