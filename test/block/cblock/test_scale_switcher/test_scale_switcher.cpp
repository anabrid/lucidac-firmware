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

blocks::CScaleSwitchFunction switcher{bus::idx_to_addr(0, bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER)};
bus::TriggerFunction switcher_sync{bus::idx_to_addr(0,bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_SYNC)};
bus::TriggerFunction switcher_clear{bus::idx_to_addr(0,bus::C_BLOCK_IDX, blocks::CBlock::SCALE_SWITCHER_CLEAR)};


void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_address() {
  //
  TEST_ASSERT_EQUAL(0b100001'0010, switcher.address);
}

void test_function() {
  switcher.data = 0b10001010'10001010'11111111'10001010;
  //switcher.data = 0b11111111'11111111'11111111'11111111;
  switcher.write_to_hardware();
  switcher_sync.trigger();

  delayMicroseconds(1);
  switcher_clear.trigger();
  switcher_sync.trigger();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_address);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}