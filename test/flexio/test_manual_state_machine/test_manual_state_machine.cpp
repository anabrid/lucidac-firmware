// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <FlexIO_t4.h>
#include <unity.h>

// FlexIO1 only has pins 1:4-7 exposed for state outputs
constexpr uint8_t PIN_STATE_OUT_FOUR = 2, PIN_STATE_OUT_FIVE = 3, PIN_STATE_OUT_SIX = 4,
                  PIN_STATE_OUT_SEVEN = 33, PIN_TEST_IN_STATE_OUT_FOUR = 23, PIN_TEST_IN_STATE_OUT_FIVE = 22,
                  PIN_TEST_IN_STATE_OUT_SIX = 21, PIN_TEST_IN_STATE_OUT_SEVEN = 20, PIN_INPUT_A = 52,
                  PIN_INPUT_B = 49, PIN_INPUT_C = 50;

// Use FlexIO 1 in this example
auto flexio = FlexIOHandler::flexIOHandler_list[0];
// Global variables for timer/shifter flexio index
uint8_t _s_zero = 0xff, _s_one = 0xff;

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
  // PIN_STATE_OUT_SIX seems rather slow
  delayNanoseconds(100);
  TEST_ASSERT_MESSAGE(digitalReadFast(pin_in), msg);
  digitalWriteFast(pin_out, LOW);
}

void test_connections() {
  test_connection(PIN_STATE_OUT_FOUR, PIN_TEST_IN_STATE_OUT_FOUR,
                  "You must connect PIN_STATE_OUT_FOUR to PIN_TEST_IN_STATE_OUT_FOUR");
  test_connection(PIN_STATE_OUT_FIVE, PIN_TEST_IN_STATE_OUT_FIVE,
                  "You must connect PIN_STATE_OUT_FIVE to PIN_TEST_IN_STATE_OUT_FIVE");
  test_connection(PIN_STATE_OUT_SIX, PIN_TEST_IN_STATE_OUT_SIX,
                  "You must connect PIN_STATE_OUT_SIX to PIN_TEST_IN_STATE_OUT_SIX");
  test_connection(PIN_STATE_OUT_SEVEN, PIN_TEST_IN_STATE_OUT_SEVEN,
                  "You must connect PIN_STATE_OUT_SEVEN to PIN_TEST_IN_STATE_OUT_SEVEN");
}

void test_flexio_setup() {
  // Trigger on this
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);

  _s_zero = flexio->requestShifter();
  _s_one = flexio->requestShifter();
  TEST_ASSERT_NOT_EQUAL(0xff, _s_zero);
  TEST_ASSERT_NOT_EQUAL(0xff, _s_one);
  TEST_ASSERT_NOT_EQUAL(_s_zero, _s_one);
  TEST_ASSERT_EQUAL_MESSAGE(0, _s_zero, "State indizes are partly hardcoded, check twice :)");
  TEST_ASSERT_EQUAL_MESSAGE(1, _s_one, "State indizes are partly hardcoded, check twice :)");

  uint8_t _input_a_flex = flexio->mapIOPinToFlexPin(PIN_INPUT_A);
  TEST_ASSERT_EQUAL_MESSAGE(12, _input_a_flex, "State inputs are partly hardcoded, check twice :)");

  // Configure state zero
  // NOTE: We don't use a timer here, only manual state transitions
  flexio->port().SHIFTCTL[_s_zero] =
      FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(_input_a_flex) | FLEXIO_SHIFTCTL_SMOD(6);
  // SHIFTCFG {PWIDTH[3:0],SSTOP[1:0],SSTART[1:0]} are respectively used to disable the output drive
  // But it's unclear whether 0 or 1 disables them :)
  // I tried both and they both don't seem to do anything haha
  // For outputs A-H: flexio->port().SHIFTCFG[_s_zero] = 0b00000000000'0HGFE'0000000'0'00'DC'00'BA;
  flexio->port().SHIFTCFG[_s_zero] = 0;
  // SHIFTBUF is [8bit outputs][24bit next state transition]
  // The 3 input bits PINSEL{+2, +1, +0} are interpreted as a number from 0 to 8 used as index,
  // which selects the next state from SHIFTBUF{2:0, 5:3, 8:6, ...}
  // Set all outputs and always change to state one
  flexio->port().SHIFTBUF[_s_zero] = 0b11111111'001'001'001'001'001'001'001'001;

  // Configure state one
  flexio->port().SHIFTCTL[_s_one] =
      FLEXIO_SHIFTCTL_PINCFG(3) | FLEXIO_SHIFTCTL_PINSEL(_input_a_flex) | FLEXIO_SHIFTCTL_SMOD(6);
  flexio->port().SHIFTCFG[_s_one] = 0;
  // Set some outputs and always change to state zero
  flexio->port().SHIFTBUF[_s_one] = 0b10101010'000'000'000'000'000'000'000'000;

  // Set IO pins to FlexIO mode
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_FOUR);
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_FIVE);
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_SIX);
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_SEVEN);

  TEST_ASSERT_TRUE(true);
  // Enable this FlexIO
  delayMicroseconds(10);
  flexio->port().CTRL = FLEXIO_CTRL_FLEXEN;
}

void test_flexio() {
  // We should start in initial state
  TEST_ASSERT_EQUAL(_s_zero, flexio->port().SHIFTSTATE);
  // Check state output, which should all be HIGH in state zero
  TEST_ASSERT(digitalReadFast(PIN_TEST_IN_STATE_OUT_FOUR));
  TEST_ASSERT(digitalReadFast(PIN_TEST_IN_STATE_OUT_FIVE));
  TEST_ASSERT(digitalReadFast(PIN_TEST_IN_STATE_OUT_SIX));
  TEST_ASSERT(digitalReadFast(PIN_TEST_IN_STATE_OUT_SEVEN));

  // Switch to next state after a bit
  delayMicroseconds(10);
  flexio->port().SHIFTSTATE = _s_one;
  delayMicroseconds(10);
  TEST_ASSERT_EQUAL(_s_one, flexio->port().SHIFTSTATE);
  // Check state output, which should be alternating in state one
  TEST_ASSERT(!digitalReadFast(PIN_TEST_IN_STATE_OUT_FOUR));
  TEST_ASSERT(digitalReadFast(PIN_TEST_IN_STATE_OUT_FIVE));
  TEST_ASSERT(!digitalReadFast(PIN_TEST_IN_STATE_OUT_SIX));
  TEST_ASSERT(digitalReadFast(PIN_TEST_IN_STATE_OUT_SEVEN));

  digitalWriteFast(LED_BUILTIN, LOW);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_connections);
  RUN_TEST(test_flexio_setup);
  RUN_TEST(test_flexio);
  UNITY_END();
}

void loop() { delay(500); }
