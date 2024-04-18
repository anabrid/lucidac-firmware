// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// 
// ANABRID_BEGIN_LICENSE
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <SPI.h>

/*
 * SPI Test
 * - clock pin 13
 * - mosi pin 11
 */

void setup() {
    SPI.begin();
}

void loop() {
    delay(50);

    SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE3));
    SPI.transfer(0xaa);
    SPI.transfer(0x77);
    SPI.endTransaction();
}