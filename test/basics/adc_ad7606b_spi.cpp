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
#include <SPI.h>


constexpr auto PIN_LED = 13;


class AD7606 {
public:
    static constexpr auto PIN_RESET = 41;
    static constexpr auto PIN_CS = 40;
    // We hijack SPI pins sometimes
    static constexpr auto PIN_CLK = 27;
    static constexpr auto PIN_MOSI = 26;
    static constexpr auto PIN_MISO = 39;

    void setup() {
        // Reset is active high
        pinMode(PIN_RESET, OUTPUT);
        digitalWriteFast(PIN_RESET, LOW);

        // CS is active low, write value before and after pinMode to not have a short dip on the line
        digitalWriteFast(PIN_CS, HIGH);
        pinMode(PIN_CS, OUTPUT);
        digitalWriteFast(PIN_CS, HIGH);



        //SPI1.begin();
    }

    void hijack_spi() {
        digitalWriteFast(PIN_CLK, HIGH);
        pinMode(PIN_CLK, OUTPUT);
        digitalWriteFast(PIN_MOSI, LOW);
        pinMode(PIN_MOSI, OUTPUT);
        pinMode(PIN_MISO, INPUT);
    }

    void set_mosi(bool value) {
        digitalWriteFast(PIN_MOSI, value);
    }

    void set_clk(bool value, bool delay = true, unsigned int delay_ns = 170) {
        if (delay)
            delayNanoseconds(delay_ns);
        digitalWriteFast(PIN_CLK, value);
        if (delay)
            delayNanoseconds(delay_ns);
    }

    uint8_t manual_transfer8(uint8_t msb_first_data) {
        uint8_t ret = 0;
        for (auto i: {7,6,5,4,3,2,1,0}) {
            set_mosi(msb_first_data & (1 << i));
            set_clk(LOW);
            set_clk(HIGH);
        }
        return ret;
    }

    void teardown() {
        //SPI1.end();
    }

    void set_cs(bool value) {
        // CS is active low, but that's left to the user.
        digitalWriteFast(PIN_CS, value);
    }

    void reset(bool full = true) {
        digitalWriteFast(PIN_RESET, HIGH);
        if (full) {
            delayMicroseconds(5);
            digitalWriteFast(PIN_RESET, LOW);
            delayMicroseconds(300);
        } else {
            delayNanoseconds(500);
            digitalWriteFast(PIN_RESET, LOW);
            delayNanoseconds(100);
        }
    }
};


void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWriteFast(PIN_LED, HIGH);

    auto adc = AD7606();
    adc.setup();
    delayMicroseconds(1);
    adc.reset();

    // Implement one register read by bit-banging, because SDI must be asserted before CS.
    // And I don't think I can do that with default SPI functions.
    // ASSUMING: ADC samples MOSI on falling CLK (datasheet page 11), MISO should be read on rising edge
    adc.hijack_spi();
    adc.set_clk(HIGH);
    adc.set_mosi(LOW);
    delayNanoseconds(100);

    adc.set_cs(LOW);

    adc.manual_transfer8(B01000000 + 0x06);
    adc.set_mosi(LOW);
    adc.manual_transfer8(B00000000);
    adc.set_mosi(LOW);

    adc.set_cs(HIGH);
    delayNanoseconds(100);
    adc.set_cs(LOW);

    adc.manual_transfer8(B01000000 + 0x06);
    adc.set_mosi(LOW);
    adc.manual_transfer8(B00000000);
    adc.set_mosi(LOW);

    adc.set_cs(HIGH);

    /*
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_mosi(HIGH); //
    adc.set_clk(LOW);   // 1
    adc.set_clk(HIGH);
    adc.set_mosi(LOW);  //
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_mosi(HIGH); //
    adc.set_clk(LOW);   // 1
    adc.set_clk(HIGH);
    adc.set_mosi(LOW);  // reset MOSI
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);

    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);

    adc.set_cs(HIGH);
    delayNanoseconds(100);
    adc.set_cs(LOW);

    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_mosi(HIGH); //
    adc.set_clk(LOW);   // 1
    adc.set_clk(HIGH);
    adc.set_mosi(LOW);  //
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_clk(LOW);   // 0
    adc.set_clk(HIGH);
    adc.set_mosi(HIGH); //
    adc.set_clk(LOW);   // 1
    adc.set_clk(HIGH);
    adc.set_mosi(LOW);  // reset MOSI

    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    adc.set_clk(LOW);
    adc.set_clk(HIGH);
    */

    delayMicroseconds(5);
    adc.teardown();

    delay(1000);
    digitalWriteFast(PIN_LED, LOW);
}


void loop() {
    delay(100);
    digitalToggleFast(PIN_LED);

}