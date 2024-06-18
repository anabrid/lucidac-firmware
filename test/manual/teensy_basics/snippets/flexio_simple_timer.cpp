// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <FlexIO_t4.h>

void setup() {
  // LED for blinking
  pinMode(13, OUTPUT);
  /*
  while (!Serial) {
      digitalToggleFast(13);
      delay(50);
  }
  */
  Serial.begin(512000);
  digitalWriteFast(13, true);

  constexpr uint8_t PIN_CLK = 4;

  Serial.println("Configuring FlexIO");
  Serial.flush();

  // Use FlexIO 1 in this example!
  auto flexio = FlexIOHandler::flexIOHandler_list[0];
  // Default clock settings are
  // flexio->setClockSettings(1 /* PLL3 480Mhz base */, 1 /* first divide by 1+1 */, 7 /* second divide by 7+1
  // */); 60 MHz FlexIO clock -> 30 MHz clock (divider again in TIMCMP)
  flexio->setClockSettings(1 /* PLL3 480Mhz base */, 1 /* first divide by 1+1=2 */,
                           3 /* second divide by 3+1=4 */);

  // Get a timer
  uint8_t _timer = flexio->requestTimers(1);
  // Get internal FlexIO pins from external pins
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK);

  // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
  // TRGSEL=0 (=default) we don't do triggering here
  // TRGPOL=1 (default=0) trigger active low
  // TRGSRC=0 (=default) we don't do triggering here
  // PINCFG=3 (default=0) set timer pin as output
  // PINSEL=clk select clp pin as output pin
  // PINPOL=0 (=default) pin is active high
  // TIMOD
  flexio->port().TIMCTL[_timer] = FLEXIO_TIMCTL_TRGPOL | FLEXIO_TIMCTL_PINCFG(3) |
                                  FLEXIO_TIMCTL_PINSEL(_sck_flex_pin) | FLEXIO_TIMCTL_TIMOD(1);

  // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
  // TIMOUT=1 (default=0) timer output is logic zero when enabled and not affected by reset
  // TIMDEC=0 (=default) decrement on FlexIO clock
  // TIMRST=0 (=default) timer never resets
  // TIMDIS=0 (=default) timer never disables
  // TIMENA=0 (=default) timer always enabled
  // TSTOP=0 (=default) timer stop bit disabled
  // TSTART=0 (=default) timer start bin disabled
  flexio->port().TIMCFG[_timer] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(0);

  // Configure timer compare
  // in dual 8-bit baud mode: [ 16 reserved ][ 8 number of bits ][ 8 clock divider as (TIMCMP[7:0] + 1)*2]
  flexio->port().TIMCMP[_timer] = 0x0000FF00;

  // Set the IO pins into FLEXIO mode
  flexio->setIOPinToFlexMode(PIN_CLK);
  // Set pin drive strength and "speed"
  constexpr uint32_t PIN_FAST_IO_CONFIG = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(2);
  *(portControlRegister(PIN_CLK)) = PIN_FAST_IO_CONFIG;

  delay(100);

  // Enable this FlexIO
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;

  /*
   * Wait until done
   */

  // TODO: use something like the following to check if transaction is done
  //       while (!(_pflex->port().SHIFTSTAT & _rx_shifter_mask) && (--timeout)) ;
  delay(2000);

  digitalWriteFast(13, false);
}

void loop() {
  // Data is only sent once in setup(), trigger with oscilloscope before flashing firmware or rewrite :)
  delay(500);
}
