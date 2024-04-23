// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public
#include "block/cblock.h"
#include "bus/functions.h"

auto base_addr = bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::COEFF_BASE_FUNC_IDX);

blocks::CBlock cblock{0};

std::array<functions::AD5452, 32> coeffs{
    functions::AD5452{base_addr, 0},  functions::AD5452{base_addr, 1},
    functions::AD5452{base_addr, 2},  functions::AD5452{base_addr, 3},
    functions::AD5452{base_addr, 4},  functions::AD5452{base_addr, 5},
    functions::AD5452{base_addr, 6},  functions::AD5452{base_addr, 7},
    functions::AD5452{base_addr, 8},  functions::AD5452{base_addr, 9},
    functions::AD5452{base_addr, 10}, functions::AD5452{base_addr, 11},
    functions::AD5452{base_addr, 12}, functions::AD5452{base_addr, 13},
    functions::AD5452{base_addr, 14}, functions::AD5452{base_addr, 15},
    functions::AD5452{base_addr, 16}, functions::AD5452{base_addr, 17},
    functions::AD5452{base_addr, 18}, functions::AD5452{base_addr, 19},
    functions::AD5452{base_addr, 20}, functions::AD5452{base_addr, 21},
    functions::AD5452{base_addr, 22}, functions::AD5452{base_addr, 23},
    functions::AD5452{base_addr, 24}, functions::AD5452{base_addr, 25},
    functions::AD5452{base_addr, 26}, functions::AD5452{base_addr, 27},
    functions::AD5452{base_addr, 28}, functions::AD5452{base_addr, 29},
    functions::AD5452{base_addr, 30}, functions::AD5452{base_addr, 31}};

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
  // coeff.data = 0 should give Vout = -Vin
  // coeff.data = 4095 << 2 should give Vout = Vin
  // coeff.data = 2047 << 2 should give Vout = 0

  // coeff.data = 1024 gives ~-1.01V
  // but does not work for {17: -4.8V}
  // at least for coeff@14, the -4.8V are independent of input signal BL_IN.14

  for (auto c : coeffs) {
    c.set_scale(static_cast<uint16_t>(1024 << 2));
  }
}

void test_via_chip_function() {
  // The f_coeff.set_scale function directly writes to hardware
  cblock.f_coeffs[0].set_scale(static_cast<uint16_t>(0));
  cblock.f_coeffs[1].set_scale(static_cast<uint16_t>(2047 << 2));
  cblock.f_coeffs[2].set_scale(static_cast<uint16_t>(4095 << 2));
}

void test_via_block() {
  TEST_ASSERT(cblock.set_factor(0, 0.5));
  // TEST_ASSERT(cblock.set_factor(0, 5));
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
