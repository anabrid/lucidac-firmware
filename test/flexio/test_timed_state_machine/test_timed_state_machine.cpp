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
uint8_t _s_zero = 0xff, _s_one = 0xff, _t_zero = 0xff, _t_one = 0xff;

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
  _s_zero = flexio->requestShifter();
  _s_one = flexio->requestShifter();
  TEST_ASSERT_NOT_EQUAL(0xff, _s_zero);
  TEST_ASSERT_NOT_EQUAL(0xff, _s_one);
  TEST_ASSERT_NOT_EQUAL(_s_zero, _s_one);
  TEST_ASSERT_EQUAL_MESSAGE(0, _s_zero, "State indizes are partly hardcoded, check twice :)");
  TEST_ASSERT_EQUAL_MESSAGE(1, _s_one, "State indizes are partly hardcoded, check twice :)");

  uint8_t _input_a_flex = flexio->mapIOPinToFlexPin(PIN_INPUT_A);
  TEST_ASSERT_EQUAL_MESSAGE(12, _input_a_flex, "State inputs are partly hardcoded, check twice :)");

  // Configure a timer to move from state zero to one
  _t_zero = flexio->requestTimers();
  TEST_ASSERT_NOT_EQUAL(0xff, _t_zero);
  flexio->port().TIMCTL[_t_zero] = FLEXIO_TIMCTL_TRGSEL(4 * _s_zero + 1) | FLEXIO_TIMCTL_TRGSRC |
                                   FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(8) | FLEXIO_TIMCTL_TIMOD(3);
  flexio->port().TIMCFG[_t_zero] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[_t_zero] = 0b0000000000000000'0000000011111111;
  flexio->setIOPinToFlexMode(5);

  // Configure state zero
  flexio->port().SHIFTCTL[_s_zero] = FLEXIO_SHIFTCTL_TIMSEL(_t_zero) | FLEXIO_SHIFTCTL_PINCFG(3) |
                                     FLEXIO_SHIFTCTL_PINSEL(_input_a_flex) | FLEXIO_SHIFTCTL_SMOD(6);
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

  // Configure a timer to move from state one to zero
  _t_one = flexio->requestTimers();
  TEST_ASSERT_NOT_EQUAL(0xff, _t_one);
  flexio->port().TIMCTL[_t_one] = FLEXIO_TIMCTL_TRGSEL(4 * _s_one + 1) | FLEXIO_TIMCTL_TRGSRC |
                                  FLEXIO_TIMCTL_PINCFG(3) | FLEXIO_TIMCTL_PINSEL(15) | FLEXIO_TIMCTL_TIMOD(3);
  flexio->port().TIMCFG[_t_one] = FLEXIO_TIMCFG_TIMDIS(6) | FLEXIO_TIMCFG_TIMENA(6);
  flexio->port().TIMCMP[_t_one] = 0b0000000000000000'0000000111100011;
  flexio->setIOPinToFlexMode(54);

  // Configure state one
  flexio->port().SHIFTCTL[_s_one] = FLEXIO_SHIFTCTL_TIMSEL(_t_one) | FLEXIO_SHIFTCTL_PINCFG(3) |
                                    FLEXIO_SHIFTCTL_PINSEL(_input_a_flex) | FLEXIO_SHIFTCTL_SMOD(6);
  flexio->port().SHIFTCFG[_s_one] = 0;
  // Set some outputs and always change to state zero
  flexio->port().SHIFTBUF[_s_one] = 0b10101010'000'000'000'000'000'000'000'000;

  // Set IO pins to FlexIO mode
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_FOUR);
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_FIVE);
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_SIX);
  flexio->setIOPinToFlexMode(PIN_STATE_OUT_SEVEN);
}

void test_flexio() {
  // Trigger on this
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);
  // Enable this FlexIO
  flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;

  // Timers are only enabled on state change, not in initial state.
  // Once we move to state one once, the loop between the states starts.
  flexio->port().SHIFTSTATE = _s_one;

  digitalWriteFast(LED_BUILTIN, LOW);
  TEST_ASSERT(true);
  delayMicroseconds(100);
  flexio->port().CTRL &= ~FLEXIO_CTRL_FLEXEN;
}

void test_flexio_live_reconfiguration() {
  // Trigger on this
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWriteFast(LED_BUILTIN, HIGH);

  // Change flexio config, set _s_zero to trigger on _t_one, which will never happen in _s_zero
  flexio->port().SHIFTCTL[_s_zero] = (flexio->port().SHIFTCTL[_s_zero] & ~FLEXIO_SHIFTCTL_TIMSEL(0x07)) | FLEXIO_SHIFTCTL_TIMSEL(_t_one);

  // Enable this FlexIO
  // Timers are still running from last test
  flexio->port().CTRL |= FLEXIO_CTRL_FLEXEN;

  // After about 40 microseconds, we are back in state zero and stuck there.
  delayMicroseconds(40);

  // While in state zero, reconfigure _t_one to increase length of state one.
  // Something like this is necessary for quantum-gatter like computations, so it's tested here.
  flexio->port().TIMCMP[_t_one] = 0b0000000000000000'0000001111100011;
  // And force state change into _s_one
  flexio->port().SHIFTSTATE = _s_one;

  delayMicroseconds(100);
  flexio->port().CTRL &= ~FLEXIO_CTRL_FLEXEN;
  digitalWriteFast(LED_BUILTIN, LOW);
  TEST_ASSERT(true);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_connections);
  RUN_TEST(test_flexio_setup);
  // Always run test_flexio and test_flexio_with_reconfiguration together!
  RUN_TEST(test_flexio);
  delay(2000);
  RUN_TEST(test_flexio_live_reconfiguration);
  UNITY_END();
}

void loop() { delay(500); }
