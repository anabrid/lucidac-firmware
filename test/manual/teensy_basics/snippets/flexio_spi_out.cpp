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
  constexpr uint8_t PIN_MOSI = 2;

  Serial.println("Configuring FlexIO");
  Serial.flush();

  // Use FlexIO 1 in this example!
  auto flexio = FlexIOHandler::flexIOHandler_list[0];

  // Get a timer and a shifter
  uint8_t _timer = flexio->requestTimers(1);
  uint8_t tx_shifter = flexio->requestShifter();
  // Get internal FlexIO pins from external pins
  uint8_t _mosi_flex_pin = flexio->mapIOPinToFlexPin(PIN_MOSI);
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK);

  Serial.println("Configuring shifter");
  Serial.flush();
  // Configure shifter (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2993)
  // PWIDTH=0 (=default) parallel shift width
  // INSRC=0 (=default) input from pin, not from other shifter
  // SSTOP=0 (=default) shifter stop bit
  // SSTART=0 (=default) shifter start bit
  flexio->port().SHIFTCFG[tx_shifter] = 0;

  // Control shifter (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2992)
  // TIMSEL=clk_timer select timer from above for shifting
  // TIMPOL=1 (default=0) shift on negative edge of shift clock
  // PINCFG=3 (default=0) set shifter pin as output (instead of open drain or similar)
  // PINSEL=mosi select mosi pin as output for this shifter
  // PINPOL=0 (=default) pin is active high
  // SMOD=2 (default=0) transmit mode, load SHIFTBUF content on expiration of the timer
  flexio->port().SHIFTCTL[tx_shifter] = FLEXIO_SHIFTCTL_TIMSEL(_timer) | FLEXIO_SHIFTCTL_TIMPOL |
                                        FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(_mosi_flex_pin) |
                                        FLEXIO_SHIFTCTL_SMOD(2);

  // Set timer compare (will be partly overwritten later)
  // See dual 8-bit counter baud mode in reference manual for details.
  flexio->port().TIMCMP[_timer] = 0x00000f01;

  // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
  // TRGSEL=1 trigger from internal shifter 0 (tx_shifter) flag (presumably when it is filled)
  // TRGPOL=1 (default=0) trigger active low
  // TRGSRC=1 (default=0) internal trigger, see TRGSEL
  // PINCFG=3 (default=0) set timer pin as output
  // PINSEL=clk select clp pin as output pin
  // PINPOL=0 (=default) pin is active high
  // TIMOD=1 (default=0) dual 8-bit counter baud mode (upper 8bit counter, lower 8bit clock divider)
  flexio->port().TIMCTL[_timer] = FLEXIO_TIMCTL_TRGSEL(1) | FLEXIO_TIMCTL_TRGPOL | FLEXIO_TIMCTL_TRGSRC |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(_sck_flex_pin) |
                                  FLEXIO_TIMCTL_TIMOD(1);

  // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
  // TIMOUT=1 (default=0) timer output is logic zero when enabled and not affected by reset
  // TIMDEC=0 (=default) decrement on FlexIO clock
  // TIMRST=0 (=default) timer never resets (we will provide new timer values whenever we use it)
  // TIMDIS=2 (default=0) timer disables on upper 8-bits match and decrement (we are in dual 8-bit mode)
  // TIMENA=2 (default=0) enabled on trigger high
  // TSTOP=0 (=default) timer stop bit disabled
  // TSTART=0 (=default) timer start bin disabled
  flexio->port().TIMCFG[_timer] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(2);

  // Enable this FlexIO
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;

  // Set the IO pins into FLEXIO mode
  flexio->setIOPinToFlexMode(PIN_MOSI);
  flexio->setIOPinToFlexMode(PIN_CLK);
  // Set pin drive strength and "speed"
  constexpr uint32_t PIN_FAST_IO_CONFIG = IOMUXC_PAD_DSE(7) | IOMUXC_PAD_SPEED(2);
  *(portControlRegister(PIN_CLK)) = PIN_FAST_IO_CONFIG;
  *(portControlRegister(PIN_MOSI)) = PIN_FAST_IO_CONFIG;

  delay(100);

  /*
   * Transaction / Timer configuration
   */

  Serial.println("Configuring timer / transaction");
  Serial.flush();
  uint32_t clock_divider = 42 / 2;
  uint8_t num_of_bits = 8;
  flexio->port().TIMCMP[_timer] = clock_divider | (num_of_bits * 2 - 1) << 8;
  delay(100);

  /*
   * Data Transfer
   */

  Serial.println("Transferring data");
  Serial.flush();
  //
  flexio->port().SHIFTBUFBIS[tx_shifter] = B10101011 << (32 - 8);

  Serial.println("Ending Transaction");
  Serial.flush();
  // flex_spi.endTransaction();

  /*
   * Wait until done
   */

  // TODO: use something like the following to check if transaction is done
  //       while (!(_pflex->port().SHIFTSTAT & _rx_shifter_mask) && (--timeout)) ;
  delay(100);

  /*
   * Deconfigure
   */

  Serial.println("Deconfiguring FlexIO");
  Serial.flush();
  flexio->freeTimers(_timer);
  _timer = 0xff;
  flexio->freeShifter(tx_shifter);
  tx_shifter = 0xff;

  digitalWriteFast(13, false);
}

void loop() {
  // Data is only sent once in setup(), trigger with oscilloscope before flashing firmware or rewrite :)
  delay(500);
}
