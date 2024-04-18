// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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

    void manual_hijack_spi() {
        digitalWriteFast(PIN_CLK, HIGH);
        pinMode(PIN_CLK, OUTPUT);
        digitalWriteFast(PIN_MOSI, LOW);
        pinMode(PIN_MOSI, OUTPUT);
        pinMode(PIN_MISO, INPUT);
    }

    void manual_set_mosi(bool value) {
        digitalWriteFast(PIN_MOSI, value);
    }

    void manual_set_clk(bool value, bool delay = true, unsigned int delay_ns = 150) {
        if (delay)
            delayNanoseconds(delay_ns);
        digitalWriteFast(PIN_CLK, value);
        if (delay)
            delayNanoseconds(delay_ns);
    }

    uint8_t sw_manual_transfer8(uint8_t msb_first_data) {
        uint8_t ret = 0;
        for (auto i: {7,6,5,4,3,2,1,0}) {
            manual_set_mosi(msb_first_data & (1 << i));
            manual_set_clk(LOW);
            ret |= digitalReadFast(PIN_MISO) << i;
            manual_set_clk(HIGH);
        }
        return ret;
    }

    uint8_t sw_manual_read_register(const uint8_t address) {
        // Assumes manual_hijack_spi has been called
        manual_set_clk(HIGH);
        manual_set_mosi(LOW);
        delayNanoseconds(100);

        manual_set_cs(LOW);

        sw_manual_transfer8(B01000000 + address);
        manual_set_mosi(LOW);
        sw_manual_transfer8(B00000000);
        manual_set_mosi(LOW);

        manual_set_cs(HIGH);
        delayNanoseconds(100);
        manual_set_cs(LOW);

        sw_manual_transfer8(B01000000 + address);
        manual_set_mosi(LOW);
        auto reg = sw_manual_transfer8(B00000000);
        manual_set_mosi(LOW);

        manual_set_cs(HIGH);
        return reg;
    }

    uint8_t sw_manual_register_(uint8_t address, uint8_t value = 0)

    void teardown() {
        //SPI1.end();
    }

    void manual_set_cs(bool value) {
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

    Serial.begin(0);
    while (!Serial && millis() < 4000) {
        // Wait for Serial to initialize
    }

    auto adc = AD7606();
    adc.setup();
    delayMicroseconds(1);
    adc.reset();

    // Implement one register read by bit-banging, because SDI must be asserted before CS.
    // And I don't think I can do that with default SPI functions.
    // ASSUMING: ADC samples MOSI on falling CLK (datasheet page 11), MISO should be read on rising edge
    adc.manual_hijack_spi();

    auto reg = adc.sw_manual_read_register(0x02);
    Serial.print("Reg 0x02: ");
    Serial.println(reg, HEX);

    reg = adc.sw_manual_read_register(0x03);
    Serial.print("Reg 0x03: ");
    Serial.println(reg, HEX);

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