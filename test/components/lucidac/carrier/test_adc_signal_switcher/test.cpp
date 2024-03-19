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
#include "carrier/carrier.h"

using namespace carrier;

Carrier carrier_board;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  TEST_ASSERT(carrier_board.init());
}

void test_adc_switcher_matrix_reset() {
  carrier_board.f_adc_switcher_matrix_reset.trigger();
}

void test_adc_prg() {
  carrier_board.f_adc_switcher_prg.transfer(functions::ICommandRegisterFunction::chip_cmd_word(3, 5));
  carrier_board.f_adc_switcher_sync.trigger();
  TEST_ASSERT(true);
}

void test_adc_sr_reset() {
  carrier_board.f_adc_switcher_sr_reset.trigger();
  TEST_ASSERT(true);
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();
  //RUN_TEST(test_init);
  RUN_TEST(test_adc_switcher_matrix_reset);
  RUN_TEST(test_adc_prg);
  //RUN_TEST(test_adc_sr_reset);
  UNITY_END();
}

void loop() {
  // Re-run tests or do an action once the button is pressed
  while (digitalReadFast(29)) {}
  test_adc_switcher_matrix_reset();
  delay(500);
}
