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

constexpr uint8_t PIN_CNVST = 4;
constexpr uint8_t PIN_SCLK = 27;
constexpr uint8_t PIN_SDO = 1;

/*
 * CONNECTION
 *  - ADC without BUSY status indicator (SDI=1)
 *  - PIN_CNVST CNVST
 *  - SCLK pin 27
 *  - MISO pin 1
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

    pinMode(PIN_SCLK, OUTPUT);
    digitalWriteFast(PIN_SCLK, LOW);

    pinMode(PIN_SDO, INPUT);
}


void loop() {
    digitalToggleFast(13);

    // One read-out
    // Trigger
    digitalWriteFast(PIN_CNVST, true);
    delayNanoseconds(500);
    digitalWriteFast(PIN_CNVST, false);
    delayNanoseconds(2000);

    // Bit-banging readout
    uint16_t value = 0;
    for (auto i : {0, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0}) {
        digitalWriteFast(PIN_SCLK, true);

        value += (digitalReadFast(PIN_SDO) << i);

        delayNanoseconds(500);
        digitalWriteFast(PIN_SCLK, false);
        delayNanoseconds(500);
    }

    // Print
    Serial.println(value);

    delay(500);
}