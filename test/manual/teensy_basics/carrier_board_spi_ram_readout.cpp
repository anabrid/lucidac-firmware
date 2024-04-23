// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// ANABRID_BEGIN_LICENSE
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
// ANABRID_END_LICENSE

#include "bus/bus.h"
#include <Arduino.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  bus::init();
  bus::address_function(0, 5, 0);
}

void loop() {
  digitalWriteFast(LED_BUILTIN, HIGH);
  // Set SPI configuration
  bus::spi.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));

  /*
   * Read out the full memory
   */
  // Set CS
  bus::address_function(0, 0, 0);
  // Send READ command
  bus::spi.transfer(B00000011);
  // Send Address
  bus::spi.transfer(B00000000);
  // Send 256*8 CLKs (data is irrelevant) to read DATA
  for (uint32_t i = 0; i < 256; i++) {
    bus::spi.transfer(B00000000);
  }
  // Unset CS
  bus::release_address();

  /*
   * Read out the 64bit unique ID memory at the end of the 256bit memory
   */
  delayMicroseconds(10);
  bus::address_function(0, 0, 0);
  // Send READ Command
  bus::spi.transfer(B00000011);
  // Send address of unique ID memory area
  bus::spi.transfer(248);
  // Send 8*8 CLKs (data is irrelevant) to read DATA
  for (uint32_t i = 0; i < 8; i++) {
    bus::spi.transfer(B00000000);
  }
  // Unset CS
  bus::release_address();

  bus::spi.endTransaction();
  digitalWriteFast(LED_BUILTIN, LOW);
  delay(200);
}
