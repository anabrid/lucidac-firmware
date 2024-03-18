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

void test_acl_prg() {
  bus::address_function(bus::address_from_tuple(Carrier::CARRIER_MADDR, Carrier::METADATA_FADDR));
  bus::activate_address();
  //carrier_board.f_acl_prg.transfer(0b11001111);
  TEST_ASSERT(true);
}

void setup() {
  bus::init();

  UNITY_BEGIN();
  //RUN_TEST(test_init);
  RUN_TEST(test_acl_prg);
  UNITY_END();
}

void loop() { delay(100); }
