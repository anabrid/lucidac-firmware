// Copyright (c) 2024 anabrid GmbH
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

#include <unity.h>

#include "bus/bus.h"

using namespace bus;
using namespace fakeit;

void setUp() {
  // This is called before *each* test.
  ArduinoFakeReset();
  When(OverloadedMethod(ArduinoFake(SPI), begin, void(void))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(Function), pinMode, void(uint8_t, uint8_t))).AlwaysReturn();
  When(OverloadedMethod(ArduinoFake(Function), digitalWrite, void(uint8_t, uint8_t))).AlwaysReturn();
  bus::init();
}

void tearDown() {
  // This is called after *each* test.
}

void test_address_register() {
  TEST_ASSERT_BITS(bus::ADDR_BITS_MASK, address_to_register(NULL_ADDRESS), GPIO6_DR);
  bus::address_function(1, 3, 7);
  TEST_ASSERT_BITS(bus::ADDR_BITS_MASK, address_to_register(idx_to_addr(1,3,7)), GPIO6_DR);
}

void test_address_register_again() {
  TEST_ASSERT_BITS(bus::ADDR_BITS_MASK, address_to_register(NULL_ADDRESS), GPIO6_DR);
  bus::address_function(static_cast<bus::addr_t>(3));
  TEST_ASSERT_BITS(bus::ADDR_BITS_MASK, 0b0000'000000'0011'00'00000000'00000000, GPIO6_DR);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(test_address_register);
  RUN_TEST(test_address_register_again);
  UNITY_END();
}
