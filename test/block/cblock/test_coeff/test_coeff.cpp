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

bus::TriggerFunction switcher_sync{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_SYNC)};
bus::TriggerFunction switcher_clear{
    bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_CLEAR)};

auto base_addr = bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::COEFF_BASE_FUNC_IDX);

std::array<blocks::CCoeffFunction, 32> coeffs{
    blocks::CCoeffFunction{base_addr, 0},  blocks::CCoeffFunction{base_addr, 1},
    blocks::CCoeffFunction{base_addr, 2},  blocks::CCoeffFunction{base_addr, 3},
    blocks::CCoeffFunction{base_addr, 4},  blocks::CCoeffFunction{base_addr, 5},
    blocks::CCoeffFunction{base_addr, 6},  blocks::CCoeffFunction{base_addr, 7},
    blocks::CCoeffFunction{base_addr, 8},  blocks::CCoeffFunction{base_addr, 9},
    blocks::CCoeffFunction{base_addr, 10}, blocks::CCoeffFunction{base_addr, 11},
    blocks::CCoeffFunction{base_addr, 12}, blocks::CCoeffFunction{base_addr, 13},
    blocks::CCoeffFunction{base_addr, 14}, blocks::CCoeffFunction{base_addr, 15},
    blocks::CCoeffFunction{base_addr, 16}, blocks::CCoeffFunction{base_addr, 17},
    blocks::CCoeffFunction{base_addr, 18}, blocks::CCoeffFunction{base_addr, 19},
    blocks::CCoeffFunction{base_addr, 20}, blocks::CCoeffFunction{base_addr, 21},
    blocks::CCoeffFunction{base_addr, 22}, blocks::CCoeffFunction{base_addr, 23},
    blocks::CCoeffFunction{base_addr, 24}, blocks::CCoeffFunction{base_addr, 25},
    blocks::CCoeffFunction{base_addr, 26}, blocks::CCoeffFunction{base_addr, 27},
    blocks::CCoeffFunction{base_addr, 28}, blocks::CCoeffFunction{base_addr, 29},
    blocks::CCoeffFunction{base_addr, 30}, blocks::CCoeffFunction{base_addr, 31}};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_address() {
  // First function on C-Block is metadata storage, first coeff has function idx 1.
  TEST_ASSERT_EQUAL(0b000001'0010, coeffs.at(0).address);
  TEST_ASSERT_EQUAL(0b000010'0010, coeffs.at(1).address);
}

void test_function() {
  switcher_clear.trigger();
  switcher_sync.trigger();
  delayMicroseconds(1);

  // coeff.data = 0 should give Vout = -Vin
  // coeff.data = 4095 << 2 should give Vout = Vin
  // coeff.data = 2047 << 2 should give Vout = 0

  // coeff.data = 1024 gives ~-1.01V
  // but does not work for {17: -4.8V}
  // at least for coeff@14, the -4.8V are independent of input signal BL_IN.14

  for (auto c : coeffs) {
    c.data = 1024 << 2;
    c.write_to_hardware();
  }

}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_address);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
