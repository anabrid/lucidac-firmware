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
#include <FlexIO_t4.h>

constexpr uint8_t PIN_CNVST = 3;
constexpr uint8_t PIN_CLK = 4; // FlexIO 1:6
constexpr uint8_t PIN_TRIGGER_IN = 5; // FlexIO 1:8

/*
 * CONNECTION
 *  - ADC _with_ BUSY status indicator (SDI=0)
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

    pinMode(PIN_CLK, OUTPUT);
    digitalWriteFast(PIN_CLK, LOW);

    pinMode(PIN_TRIGGER_IN, INPUT_PULLUP);

    Serial.println("Configuring FlexIO");
    Serial.flush();

    // Use FlexIO 1 in this example
    auto flexio = FlexIOHandler::flexIOHandler_list[0];
    flexio->setClockSettings(1 /* PLL3 480Mhz base */, 3 /* first divide */, 3 /* second divide */);

    // Get a timer
    uint8_t _timer = flexio->requestTimers(1);
    // Get internal FlexIO pins from external pins
    uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK);
    uint8_t _trigger_flex_pin = flexio->mapIOPinToFlexPin(PIN_TRIGGER_IN);

    // Set timer compare
    // See dual 8-bit counter baud mode in reference manual for details.
    // [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit divider ]
    flexio->port().TIMCMP[_timer] = 0x00001f04;

    // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
    flexio->port().TIMCTL[_timer] = FLEXIO_TIMCTL_TRGSEL(2*_trigger_flex_pin) |  FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_TRGPOL |
                                    FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(_sck_flex_pin) |
                                    FLEXIO_TIMCTL_TIMOD(1);

    // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
    flexio->port().TIMCFG[_timer] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(2);

    // Set the IO pins into FLEXIO mode
    flexio->setIOPinToFlexMode(PIN_CLK);
    flexio->setIOPinToFlexMode(PIN_TRIGGER_IN);
    // Set pin drive strength and "speed"
    constexpr uint32_t PIN_FAST_IO_CONFIG = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(2);
    *(portControlRegister(PIN_CLK)) = PIN_FAST_IO_CONFIG;
    *(portControlRegister(PIN_TRIGGER_IN)) = PIN_FAST_IO_CONFIG | IOMUXC_PAD_PKE | IOMUXC_PAD_PUE | IOMUXC_PAD_PUS(3) | IOMUXC_PAD_HYS;

    delay(100);

    // Enable this FlexIO
    flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;

    digitalWriteFast(13, false);
    delay(100);
}


void loop() {
    digitalToggleFast(13);

    // One read-out
    // Trigger
    digitalWriteFast(PIN_CNVST, true);
    delayNanoseconds(200);
    digitalWriteFast(PIN_CNVST, false);

    // FlexIO timer will trigger on ADC busy signal and output SCLK

    delay(500);
}