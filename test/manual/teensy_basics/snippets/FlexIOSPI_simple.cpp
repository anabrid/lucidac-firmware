// Based on
// https://github.com/KurtE/FlexIO_t4/blob/33d60d4751e83e9ee2055726c40f0b4e35f4fa20/examples/SPI/Simple/Simple.ino
// Copyright (c) 2021 Kurt E
// SPDX-License-Identifier: MIT

#include <FlexIOSPI.h>
#include <FlexIO_t4.h>

//#define HARDWARE_CS
#ifdef HARDWARE_CS
FlexIOSPI SPIFLEX(2, 3, 4, 5); // Setup on (int mosiPin, int sckPin, int misoPin, int csPin=-1) :
#define assert_cs()
#define release_cs()
#else
FlexIOSPI SPIFLEX(2, 3, 4, -1); // Setup on (int mosiPin, int sckPin, int misoPin, int csPin=-1) :
#define assert_cs() digitalWriteFast(5, LOW)
#define release_cs() digitalWriteFast(5, HIGH)
#endif

void setup() {
  pinMode(13, OUTPUT);
  // while (!Serial && millis() < 4000);
  Serial.begin(115200);
  delay(500);
#ifndef HARDWARE_CS
  pinMode(5, OUTPUT);
  release_cs();
#endif
  if (!SPIFLEX.begin()) {
    Serial.println("SPIFlex Begin Failed");
  }

  // See if we can update the speed...
  // SPIFLEX.flexIOHandler()->setClockSettings(2, 1, 7);	// clksel(0-3PLL4, Pll3 PFD2 PLL5, *PLL3_sw)

  Serial.printf("Updated Flex IO speed: %u\n", SPIFLEX.flexIOHandler()->computeClockRate());

  Serial.println("End Setup");
}

void loop() {
  SPIFLEX.beginTransaction(FlexIOSPISettings(1000000, MSBFIRST, SPI_MODE0));

  assert_cs();
  SPIFLEX.transfer(B10101011);
  release_cs();
  Serial.println("After Transfer loop");

  SPIFLEX.endTransaction();
  delay(500);
}
