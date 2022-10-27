// Copyright (c) 2022 anabrid GmbH
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