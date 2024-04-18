// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <FlexIO_t4.h>
#include <bitset>
#include <unity.h>

constexpr uint8_t PIN_DATA_IN = 3; // FlexIO 1:5
constexpr uint8_t PIN_DATA_OUT = 20;
constexpr uint8_t PIN_CLK = 4;        // FlexIO 1:6
constexpr uint8_t PIN_TRIGGER_IN = 5; // FlexIO 1:8
constexpr uint8_t PIN_TRIGGER_OUT = 21;

// Use FlexIO 1 in this example
auto flexio = FlexIOHandler::flexIOHandler_list[0];
// Global variables for timer/shifter flexio index
uint8_t _timer = 0xff;
uint8_t _shifter = 0xff;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_connection(uint8_t pin_out, uint8_t pin_in, const char *msg) {
  TEST_ASSERT_NOT_EQUAL(pin_out, pin_in);
  pinMode(pin_out, OUTPUT);
  pinMode(pin_in, INPUT);
  digitalWriteFast(pin_out, HIGH);
  TEST_ASSERT_MESSAGE(digitalReadFast(pin_in), msg);
  digitalWriteFast(pin_out, LOW);
}

void test_connections() {
  test_connection(PIN_TRIGGER_OUT, PIN_TRIGGER_IN, "You must connect TRIGGER_OUT to TRIGGER_IN");
  test_connection(PIN_DATA_OUT, PIN_DATA_IN, "You must connect PIN_DATA_OUT to PIN_DATA_IN");
}

void test_flexio_setup() {
  // Get a timer
  _timer = flexio->requestTimers(1);
  TEST_ASSERT_NOT_EQUAL(0xff, _timer);
  // Get internal FlexIO pins from external pins
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK);
  TEST_ASSERT_NOT_EQUAL(0xff, _sck_flex_pin);
  uint8_t _trigger_flex_pin = flexio->mapIOPinToFlexPin(PIN_TRIGGER_IN);
  TEST_ASSERT_NOT_EQUAL(0xff, _trigger_flex_pin);

  // Set timer compare
  // See dual 8-bit counter baud mode in reference manual for details.
  // [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit divider ]
  flexio->port().TIMCMP[_timer] = 0x0000'40'01;

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

  // Get a shifter
  _shifter = flexio->requestShifter();
  TEST_ASSERT_NOT_EQUAL(0xff, _shifter);
  // Get FlexIO pins
  auto _flexio_pin_data_in = flexio->mapIOPinToFlexPin(PIN_DATA_IN);
  TEST_ASSERT_NOT_EQUAL(0xff, _flexio_pin_data_in);
  // Configure shifter for simple receive
  flexio->port().SHIFTCTL[_shifter] =
      FLEXIO_SHIFTCTL_TIMSEL(_timer) | FLEXIO_SHIFTCTL_PINSEL(_flexio_pin_data_in) | FLEXIO_SHIFTCTL_SMOD(1);
  flexio->port().SHIFTCFG[_shifter] = 0;
  // Set IO pin to FlexIO mode
  flexio->setIOPinToFlexMode(PIN_DATA_IN);

  delay(100);

  // Enable this FlexIO
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;
}

void trigger() {
  digitalWriteFast(PIN_TRIGGER_OUT, HIGH);
  delayNanoseconds(100);
  digitalWriteFast(PIN_TRIGGER_OUT, LOW);
}

void test_flexio() {
  // Trigger capture
  trigger();
  delayMicroseconds(10);

  // Timer should have expired, thus SHIFTSTAT should be set and SHIFTBUF should be all zero
  TEST_ASSERT_EQUAL(1 << _shifter, flexio->port().SHIFTSTAT);
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTBUF[_shifter]);
  // Shiftstat is cleared by read
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTSTAT);

  // Trigger capture again, but change PIN_DATA_OUT in between
  pinMode(PIN_DATA_OUT, OUTPUT);
  trigger();
  delayMicroseconds(1);
  digitalWriteFast(PIN_DATA_OUT, HIGH);
  delayMicroseconds(10);
  digitalWriteFast(PIN_DATA_OUT, LOW);

  // Timer should have expired, thus SHIFTSTAT should be set and SHIFTBUF should be partly ones
  TEST_ASSERT_EQUAL(1 << _shifter, flexio->port().SHIFTSTAT);
  TEST_ASSERT_EQUAL(0b0000'0000'1111'1111'1111'1111'1111'1111, flexio->port().SHIFTBUFBIS[_shifter]);
  // Shiftstat is cleared by read
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTSTAT);

  // Trigger capture again, but change PIN_DATA_OUT in between
  pinMode(PIN_DATA_OUT, OUTPUT);
  trigger();
  delayMicroseconds(3);
  digitalWriteFast(PIN_DATA_OUT, HIGH);
  delayMicroseconds(10);
  digitalWriteFast(PIN_DATA_OUT, LOW);

  // Timer should have expired, thus SHIFTSTAT should be set and SHIFTBUF should be partly ones
  TEST_ASSERT_EQUAL(1 << _shifter, flexio->port().SHIFTSTAT);
  TEST_ASSERT_EQUAL(0b0000'0000'0000'0000'0000'0001'1111'1111, flexio->port().SHIFTBUFBIS[_shifter]);
  // Shiftstat is cleared by read
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTSTAT);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_connections);
  RUN_TEST(test_flexio_setup);
  RUN_TEST(test_flexio);
  UNITY_END();
}

void loop() { delay(1); }
