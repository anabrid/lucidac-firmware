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

#define protected public
#define private public
#include "block/cblock.h"
#include "bus/functions.h"

functions::SR74HCT595 switcher_shift_reg{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER)};
functions::TriggerFunction switcher_sync{bus::idx_to_addr(0,bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_SYNC)};
functions::TriggerFunction switcher_clear{bus::idx_to_addr(0,bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_CLEAR)};

blocks::CBlock cblock{0};

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
  TEST_ASSERT_EQUAL(0b100001'0010, switcher_shift_reg.address);
}

void test_function() {
  switcher_shift_reg.transfer32(0b10001010'10001010'11111111'10001010);
  //switcher_shift_reg.data = 0b11111111'11111111'11111111'11111111;
  switcher_sync.trigger();

  //delayMicroseconds(1);
  //switcher_clear.trigger();
  //switcher_sync.trigger();
}

void test_via_block() {
  cblock.set_upscaling(0, true);
  cblock.set_upscaling(7, true);
  cblock.set_upscaling(27, true);
  TEST_ASSERT_EQUAL(0b00001000'00000000'00000000'10000001, cblock.upscaling_);
  cblock.write_upscaling_to_hardware();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_address);
  RUN_TEST(test_function);
  RUN_TEST(test_via_block);
  UNITY_END();
}

void loop() {}