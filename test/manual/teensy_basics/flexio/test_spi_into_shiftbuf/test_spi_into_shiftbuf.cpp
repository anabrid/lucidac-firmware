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
  // Get a timer
  _timer = flexio->requestTimers(1);
  TEST_ASSERT_NOT_EQUAL(0xff, _timer);
  // Get internal FlexIO pins from external pins
  uint8_t _sck_flex_pin = flexio->mapIOPinToFlexPin(PIN_CLK_IN);
  TEST_ASSERT_NOT_EQUAL(0xff, _sck_flex_pin);

  // Set timer compare
  // See dual 8-bit counter baud mode in reference manual for details.
  // [ 16 bit reserved ][ 8 bit number of bits * 2 - 1 ][ 8 bit divider ]
  flexio->port().TIMCMP[_timer] = 0x0000'3F'00;
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
  flexio->port().SHIFTCTL[_shifter] =
      FLEXIO_SHIFTCTL_TIMSEL(_timer) | FLEXIO_SHIFTCTL_PINSEL(_flexio_pin_data_in) | FLEXIO_SHIFTCTL_SMOD(1);
  flexio->port().SHIFTCFG[_shifter] = 0;
  // Set IO pin to FlexIO mode
  flexio->setIOPinToFlexMode(PIN_DATA_IN);

  delay(100);

  // Enable this FlexIO
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;
}

void test_flexio() {
  SPI1.begin();
  SPI1.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));

  // Send all zeros
  SPI1.transfer(0);
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);
  SPI1.transfer(0);
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);
  SPI1.transfer(0);
  TEST_ASSERT_BITS_LOW(1 << _timer, flexio->port().TIMSTAT);
  SPI1.transfer(0);

  // Timer should have expired, thus TIMSTAT & SHIFTSTAT should be set and SHIFTBUF should be all zeros
  TEST_ASSERT_BITS_HIGH(1 << _timer, flexio->port().TIMSTAT);
  TEST_ASSERT_BITS_HIGH(1 << _shifter, flexio->port().SHIFTSTAT);
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTBUF[_shifter]);
  // Shiftstat is cleared by read
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTSTAT);

  // Send all ones
  SPI1.transfer32(0b11111111'11111111'11111111'11111111);

  // Timer should have expired, thus TIMSTAT & SHIFTSTAT should be set and SHIFTBUF should be all ones
  TEST_ASSERT_EQUAL(1 << _timer, flexio->port().TIMSTAT);
  TEST_ASSERT_EQUAL(1 << _shifter, flexio->port().SHIFTSTAT);
  uint32_t value = flexio->port().SHIFTBUF[_shifter];
  TEST_MESSAGE(std::bitset<32>(value).to_string().c_str());
  TEST_ASSERT_EQUAL(0b11111111'11111111'11111111'11111111, value);
  // Shiftstat is cleared by read
  TEST_ASSERT_EQUAL(0, flexio->port().SHIFTSTAT);

  // Send arbitrary data
  SPI1.transfer32(0b11000111'10101010'11111111'00000000);

  // Timer should have expired, thus TIMSTAT & SHIFTSTAT should be set and SHIFTBUF should be the sent data
  TEST_ASSERT_EQUAL(1 << _timer, flexio->port().TIMSTAT);
  TEST_ASSERT_EQUAL(1 << _shifter, flexio->port().SHIFTSTAT);
  value = flexio->port().SHIFTBUF[_shifter];
  TEST_MESSAGE(std::bitset<32>(value).to_string().c_str());
  // SHIFTBUF is bit-reversed, one could use SHIFTBUFBIS instead
  TEST_ASSERT_EQUAL(0b00000000'11111111'01010101'11100011, value);
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
