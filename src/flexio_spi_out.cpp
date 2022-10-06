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
#include <FlexIOSPI.h>
#include <FlexIO_t4.h>


void setup() {
    // LED for blinking
    pinMode(13, OUTPUT);
    while (!Serial) {
        digitalToggleFast(13);
        delay(50);
    }
    Serial.begin(512000);
    digitalWriteFast(13, true);

    Serial.println("Configuring FlexIO");
    Serial.flush();
    FlexIOSPI flex_spi(2, 3, 4, -1); // Setup on (int mosiPin, int sckPin, int misoPin, int csPin=-1) :
    if (!flex_spi.begin()) {
        Serial.println("SPIFlex Begin Failed");
    }
    delay(100);

    Serial.println("Configuring Transaction");
    Serial.flush();
    flex_spi.beginTransaction(FlexIOSPISettings(1000000, MSBFIRST, SPI_MODE0));
    delay(100);

    Serial.println("Transferring data");
    Serial.flush();
    flex_spi.transfer(B10101010);

    Serial.println("Ending Transaction");
    Serial.flush();
    flex_spi.endTransaction();

    Serial.println("Deconfiguring FlexIO");
    Serial.flush();
    flex_spi.end();

    /*
    // Use FlexIO2
    Serial.println("Getting FlexIO handler");
    Serial.flush();
    FlexIOHandler *flexio = FlexIOHandler::flexIOHandler_list[1];

    constexpr uint8_t CLK_PIN = 4;
    constexpr uint8_t MOSI_PIN = 2;
    // CLK
    //pinMode(CLK_PIN, OUTPUT);
    // MOSI
    //pinMode(MOSI_PIN, OUTPUT);

    // Map to corresponding FlexIO2:n pin
    Serial.println("Mapping to internal FlexIO2 pins");
    Serial.flush();
    auto clk_flexio_pin = flexio->mapIOPinToFlexPin(CLK_PIN);
    auto mosi_flexio_pin = flexio->mapIOPinToFlexPin(MOSI_PIN);

    // Get a timer and a shifter
    Serial.println("Requesting timers");
    Serial.flush();
    auto clk_timer = flexio->requestTimers(1);
    Serial.println("Requesting shifter");
    Serial.flush();
    auto tx_shifter = flexio->requestShifter();

    Serial.println("Configuring shifter");
    Serial.flush();
    // Configure shifter (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2993)
    // PWIDTH=0 (=default) parallel shift width
    // INSRC=0 (=default) input from pin, not from other shifter
    // SSTOP=0 (=default) shifter stop bit
    // SSTART=0 (=default) shifter start bit
    flexio->port().SHIFTCFG[tx_shifter] = 0;

    digitalWriteFast(13, false); delay(100);
    // Control shifter (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2992)
    // TIMSEL=clk_timer select timer from above for shifting
    // TIMPOL=1 (default=0) shift on negative edge of shift clock
    // PINCFG=3 (default=0) set shifter pin as output (instead of open drain or similar)
    // PINSEL=mosi select mosi pin as output for this shifter
    // PINPOL=0 (=default) pin is active high
    // SMOD=2 (default=0) transmit mode, load SHIFTBUF content on expiration of the timer
    flexio->port().SHIFTCTL[tx_shifter] = FLEXIO_SHIFTCTL_TIMSEL(clk_timer) | FLEXIO_SHIFTCTL_TIMPOL |
                                          FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(mosi_flexio_pin) |
                                          FLEXIO_SHIFTCTL_SMOD(2);

    // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
    // TRGSEL=1 trigger from internal shifter 0 (tx_shifter) flag (presumably when it is filled)
    // TRGPOL=1 (default=0) trigger active low
    // TRGSRC=1 (default=0) internal trigger, see TRGSEL
    // PINCFG=3 (default=0) set timer pin as output
    // PINSEL=clk select clp pin as output pin
    // PINPOL=0 (=default) pin is active high
    // TIMOD=1 (default=0) dual 8-bit counter baud mode (upper 8bit counter, lower 8bit clock divider)
    flexio->port().TIMCTL[clk_timer] = FLEXIO_TIMCTL_TRGSEL(1) | FLEXIO_TIMCTL_TRGPOL | FLEXIO_TIMCTL_TRGSRC |
                                       FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(clk_flexio_pin) |
                                       FLEXIO_TIMCTL_TIMOD(1);
    // Set timer compare (will be partly overwritten later)
    // See dual 8-bit counter baud mode in reference manual for details.
    flexio->port().TIMCMP[clk_timer] = 0x00000f01;
    // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
    // TIMOUT=1 (default=0) timer output is logic zero when enabled and not affected by reset
    // TIMDEC=0 (=default) decrement on FlexIO clock
    // TIMRST=0 (=default) timer never resets (we will provide new timer values whenever we use it)
    // TIMDIS=2 (default=0) timer disables on upper 8-bits match and decrement (we are in dual 8-bit mode)
    // TIMENA=2 (default=0) enabled on trigger high
    // TSTOP=0 (=default) timer stop bit disabled
    // TSTART=0 (=default) timer start bin disabled
    flexio->port().TIMCFG[clk_timer] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(2);

    // Enable FlexIO
    flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;

    // Set pins to FlexIO mode
    flexio->setIOPinToFlexMode(CLK_PIN);
    flexio->setIOPinToFlexMode(MOSI_PIN);
    // Set pin drive strength and "speed"
    constexpr uint32_t PIN_FAST_IO_CONFIG = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(2);
    *(portControlRegister(CLK_PIN)) = PIN_FAST_IO_CONFIG;
    *(portControlRegister(MOSI_PIN)) = PIN_FAST_IO_CONFIG;

    // ! FlexIOSPI library adds a IO handler callback here, we don't.


    // A fixed beginTransaction(...)
    // Set number of bits to transfer and clock divider
    uint32_t clock_divider = 100;
    uint8_t num_of_bits = 8;
    flexio->port().TIMCMP[clk_timer] = clock_divider | (num_of_bits * 2 - 1) << 8;

    // Send data once
    flexio->port().SHIFTBUF[tx_shifter] = 0xFFFFFFFF;
    */
}


void loop() {
    // Data is only sent once in setup(), trigger with oscilloscope before flashing firmware or rewrite :)
    delay(500);
}