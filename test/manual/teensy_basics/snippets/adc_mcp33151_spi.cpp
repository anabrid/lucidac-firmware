// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <SPI.h>

constexpr uint8_t PIN_CNVST = 4;

/*
 * CONNECTION
 *  - ADC without BUSY status indicator (SDI=1)
 *  - PIN_CNVST CNVST
 *  - SPI1 SCLK pin 27
 *  - SPI1 MISO pin 1
 */

void setup() {
  pinMode(13, OUTPUT);

  // Wait for serial connection
  /*
  while (!Serial) {
      digitalToggleFast(13);
      delay(100);
  }
  */
  Serial.begin(512000);
  Serial.println("Manual ADC tests.");

  // Set CNVST default low
  pinMode(PIN_CNVST, OUTPUT);
  digitalWriteFast(PIN_CNVST, LOW);

  // Initialize SPI1, since SPI0 conflicts with LED
  SPI1.begin();
  SPI1.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
}

void loop() {
  digitalToggleFast(13);

  // One read-out
  // Trigger
  digitalWriteFast(PIN_CNVST, true);
  delayNanoseconds(500);
  digitalWriteFast(PIN_CNVST, false);
  delayNanoseconds(2000);
  // SPI readout
  auto value = SPI1.transfer16(0);
  // Print
  Serial.println(value);

  delay(500);
}
