// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <FlexIO_t4.h>
#include <SPI.h>
#include <bitset>
#include <unity.h>

constexpr uint8_t PIN_DATA_IN = 3; // FlexIO 1:5
constexpr uint8_t PIN_MOSI1_OUT = 26;
constexpr uint8_t PIN_CLK_IN = 4;    // FlexIO 1:6
constexpr uint8_t PIN_SCK1_OUT = 27; // SPI1 CLK
constexpr uint8_t PIN_TRIGGER_OUT = 5;

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
  test_connection(PIN_MOSI1_OUT, PIN_DATA_IN, "You must connect PIN_MOSI1_OUT to PIN_DATA_IN");
  test_connection(PIN_SCK1_OUT, PIN_CLK_IN, "You must connect PIN_SCK1_OUT to PIN_CLK_IN");
}

void test_flexio_setup() {
  // With a faster clock you get faster synchronization
  // flexio->setClockSettings(3,0,0);

  // Get a timer
  _timer = flexio->requestTimers(1);
  TEST_ASSERT_NOT_EQUAL(0xff, _timer);
  // Get internal FlexIO pins from external pins
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK_IN);
  TEST_ASSERT_NOT_EQUAL(0xff, _sck_flex_pin);

  // Set timer compare
  // TODO: Replace the dual 8-bit baud mode by a timer that does not expire
  // See dual 8-bit counter baud mode in reference manual for details.
  // [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit divider ]
  flexio->port().TIMCMP[_timer] = 0x0000'FF'00;
  // Control timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.2999)
  flexio->port().TIMCTL[_timer] = FLEXIO_TIMCTL_PINSEL(_sck_flex_pin) | FLEXIO_TIMCTL_TIMOD(1);
  // Configure timer (https://www.pjrc.com/teensy/IMXRT1060RM_rev3.pdf p.3002)
  flexio->port().TIMCFG[_timer] = FLEXIO_TIMCFG_TIMOUT(1) | FLEXIO_TIMCFG_TIMDEC(2);
  // Set the IO pins into FLEXIO mode
  flexio->setIOPinToFlexMode(PIN_CLK_IN);

  // Get a shifter
  _shifter = flexio->requestShifter();
  TEST_ASSERT_NOT_EQUAL(0xff, _shifter);
  // Get FlexIO pins
  auto _flexio_pin_data_in = flexio->mapIOPinToFlexPin(PIN_DATA_IN);
  TEST_ASSERT_NOT_EQUAL(0xff, _flexio_pin_data_in);
  // Configure shifter for simple receive
  flexio->port().SHIFTCTL[_shifter] = FLEXIO_SHIFTCTL_TIMSEL(_timer) |
                                      FLEXIO_SHIFTCTL_PINSEL(_flexio_pin_data_in) |
                                      FLEXIO_SHIFTCTL_SMOD(0b101);
  flexio->port().SHIFTCFG[_shifter] = 0;
  // Set compare value in SHIFTBUF[31:16] and mask in SHIFTBUF[15:0] (1=mask, 0=no mask)
  // If you set the first 16 bits as compare value to all zeros, the shifter immediately matches.
  // If you set the last 16 bits as mask to all ones, the shifter immediately matches, meaning entries with
  // 1=mask are really masked, i.e. matched regardless of their value
  flexio->port().SHIFTBUF[_shifter] = 0b01010101'00001111'00000000'00000000;
  // Reset SHIFTERR (=a match has occurred at any time) by writing a 1, as it may be set from previous runs
  flexio->port().SHIFTERR |= 1 << _shifter;
  // Set IO pin to FlexIO mode
  flexio->setIOPinToFlexMode(PIN_DATA_IN);

  // Get a timer which is enabled when there is a match
  auto _triggered_timer = flexio->requestTimers();
  TEST_ASSERT_NOT_EQUAL(_timer, _triggered_timer);
  TEST_ASSERT_NOT_EQUAL(0xff, _triggered_timer);
  auto _flexio_pin_trigger_out = flexio->mapIOPinToFlexPin(PIN_TRIGGER_OUT);
  TEST_ASSERT_NOT_EQUAL(0xff, _flexio_pin_trigger_out);
  // Configure timer
  flexio->port().TIMCTL[_triggered_timer] =
      FLEXIO_TIMCTL_TRGSEL(4 * _shifter + 1) | FLEXIO_TIMCTL_TRGSRC | FLEXIO_TIMCTL_PINCFG(3) |
      FLEXIO_TIMCTL_PINSEL(_flexio_pin_trigger_out) | FLEXIO_TIMCTL_TIMOD(1);
  flexio->port().TIMCFG[_triggered_timer] = FLEXIO_TIMCFG_TIMDIS(2) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[_triggered_timer] = 0x0000'08'00;
  // Set IO pin to FlexIO mode
  flexio->setIOPinToFlexMode(PIN_TRIGGER_OUT);

  // Enable this FlexIO
  delay(100);
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;
}

void test_flexio() {
  // At the beginning, no status should be set
  // Timer should not have expired
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);
  // SHIFTSTAT = current value is matched, should not be set
  TEST_ASSERT_BITS_LOW(1 << _shifter, flexio->port().SHIFTSTAT);
  // SHIFTERR = a match has occurred at any time, should not be set (this is cleared in setup)
  TEST_ASSERT_BITS_LOW(1 << _shifter, flexio->port().SHIFTERR);

  SPI1.begin();
  SPI1.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));

  // Send 16 zeros
  SPI1.transfer16(0b00000000'00000000);

  // Timer should have expired, thus TIMSTAT should be set
  // But comparison should not have been successful, thus SHIFTSTAT & SHIFTERR should not be set
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);
  TEST_ASSERT_BITS_LOW(1 << _shifter, flexio->port().SHIFTSTAT);
  TEST_ASSERT_BITS_LOW(1 << _shifter, flexio->port().SHIFTERR);
  // Reset TIMSTAT by writing a 1, which is not done automatically
  flexio->port().TIMSTAT &= (1 << _timer);
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);

  // Send a bunch of matching data
  // This is shifted into SHIFTBUF in reverse
  SPI1.transfer16(0b11110000'10101010);

  // Timer should have expired and shifter should have matched
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);
  TEST_ASSERT_BITS_HIGH(1 << _shifter, flexio->port().SHIFTERR);
  TEST_ASSERT_BITS_HIGH(1 << _shifter, flexio->port().SHIFTSTAT);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_connections);
  RUN_TEST(test_flexio_setup);
  RUN_TEST(test_flexio);
  UNITY_END();
}

void loop() {
  delay(500);
  pinMode(LED_BUILTIN, OUTPUT);
  digitalToggle(LED_BUILTIN);
}
