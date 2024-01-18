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
#define protected public
#include "block/block.h"
#include "block/cblock.h"
#include "bus/functions.h"

functions::TriggerFunction switcher_sync{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_SYNC)};
functions::TriggerFunction switcher_clear{
    bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_CLEAR)};

auto base_addr = bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::COEFF_BASE_FUNC_IDX);

blocks::CBlock cblock{0};

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
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Put start-up sequence into a test case, so we can assert it worked.
  bus::init();
  TEST_ASSERT(cblock.init());
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

void test_via_chip_function() {
  // The f_coeff.set_scale function directly writes to hardware
  cblock.f_coeffs[0].set_scale(static_cast<uint16_t>(0));
  cblock.f_coeffs[1].set_scale(static_cast<uint16_t>(2047 << 2));
  cblock.f_coeffs[2].set_scale(static_cast<uint16_t>(4095 << 2));
  // Upscaling can also be handled manually
  cblock.f_upscaling.transfer32(0b00000000'00000000'00000000'00000000);
  cblock.f_upscaling_sync.trigger();
}

void test_via_block() {
  TEST_ASSERT(cblock.set_factor(0, 0.5));
  //TEST_ASSERT(cblock.set_factor(0, 5));
  cblock.write_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_address);
  RUN_TEST(test_function);
  RUN_TEST(test_via_chip_function);
  RUN_TEST(test_via_block);
  UNITY_END();
}

void loop() {}
