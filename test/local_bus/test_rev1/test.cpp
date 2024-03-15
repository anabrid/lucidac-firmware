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
  static uint8_t addr = 0;

  bus::spi.beginTransaction(SPISettings(10'000, MSBFIRST, SPI_MODE2));
  digitalWriteFast(PIN_ADDR_CS, LOW);
  delayMicroseconds(2);
  // MSBFIRST [8bit] = ADDR_[xx543210]
  bus::spi.transfer(0b11001101);
  // MSBFIRST [8bit] = MADDR_[xxx43210]
  bus::spi.transfer(0b11111111);
  delayMicroseconds(2);
  digitalWriteFast(PIN_ADDR_CS, HIGH);
  bus::spi.endTransaction();
  delayMicroseconds(2);

  // function beginTransaction
  digitalToggleFast(PIN_ADDR_LATCH);
  delayNanoseconds(200);
  digitalToggleFast(PIN_ADDR_LATCH);
  delayMicroseconds(2);

  bus::spi.transfer(0b10010011);
  delayMicroseconds(2);

  // release address
  // function endTransaction
  addr += 1;
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
