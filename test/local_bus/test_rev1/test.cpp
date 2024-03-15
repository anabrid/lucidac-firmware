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

#include "bus/bus.h"

using namespace bus;

void setUp() {
  // This is called before *each* test.
  bus::init();
}

void tearDown() {
  // This is called after *each* test.
}

void test_address_shiftregister() {
  address_function(0b00110011'00010111);
  delayMicroseconds(2);

  // function beginTransaction
  bus::spi.beginTransaction(SPISettings(1'000'000, LSBFIRST, SPI_MODE0));
  // activate address
  activate_address();
  delayMicroseconds(2);
  // data transfer
  bus::spi.transfer(0b10010011);
  delayMicroseconds(2);
  // release address
  deactivate_address();
  // function endTransaction
  bus::spi.endTransaction();
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_address_shiftregister);
  UNITY_END();
}

void loop() {
  delay(333);
  //test_address_shiftregister();
}
