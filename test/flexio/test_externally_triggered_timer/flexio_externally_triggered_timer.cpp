// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <FlexIO_t4.h>
#include <unity.h>

constexpr uint8_t PIN_CLK = 4;        // FlexIO 1:6
constexpr uint8_t PIN_TRIGGER_IN = 5; // FlexIO 1:8
constexpr uint8_t PIN_TRIGGER_OUT = 21;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_connections() {
  pinMode(PIN_TRIGGER_IN, INPUT);
  pinMode(PIN_TRIGGER_OUT, OUTPUT);
  digitalWriteFast(PIN_TRIGGER_OUT, HIGH);
  TEST_ASSERT_MESSAGE(digitalReadFast(PIN_TRIGGER_IN), "You must connect TRIGGER_OUT to TRIGGER_IN");
  digitalWriteFast(PIN_TRIGGER_OUT, LOW);
}

void test_flexio_setup() {
  // Use FlexIO 1 in this example
  auto flexio = FlexIOHandler::flexIOHandler_list[0];

  // Get a timer
  uint8_t _timer = flexio->requestTimers(1);
  TEST_ASSERT_NOT_EQUAL(0xff, _timer);
  // Get internal FlexIO pins from external pins
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK);
  TEST_ASSERT_NOT_EQUAL(0xff, _sck_flex_pin);
  uint8_t _trigger_flex_pin = flexio->mapIOPinToFlexPin(PIN_TRIGGER_IN);
  TEST_ASSERT_NOT_EQUAL(0xff, _trigger_flex_pin);

  // Set timer compare
  // See dual 8-bit counter baud mode in reference manual for details.
  // [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit divider ]
  flexio->port().TIMCMP[_timer] = 0x00001f01;

  // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
  flexio->port().TIMCTL[_timer] = FLEXIO_TIMCTL_TRGSEL(2 * _trigger_flex_pin) | FLEXIO_TIMCTL_TRGSRC |
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
  *(portControlRegister(PIN_TRIGGER_IN)) = PIN_FAST_IO_CONFIG;

  delay(100);

  // Enable this FlexIO
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_connections);
  RUN_TEST(test_flexio_setup);
  UNITY_END();
}

void loop() {
  delay(1);

  // Trigger clock
  digitalWriteFast(PIN_TRIGGER_OUT, HIGH);
  delayNanoseconds(100);
  digitalWriteFast(PIN_TRIGGER_OUT, LOW);
}