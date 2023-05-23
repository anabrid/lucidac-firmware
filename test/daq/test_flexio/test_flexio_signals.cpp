// From https://github.com/manitou48/teensy4/blob/master/flexiopwm.ino

// flexio pwm  from SDK
// flexio2 clock 480 mhz.  pin 12 2:1   TIMER 0   no shifter

#include <Arduino.h>
#include <DMAChannel.h>
#include <FlexIO_t4.h>
#include <array>
#include <unity.h>

// Include daq.h with all private members made public,
// to inspect internal variables without additional hassle.
#define private public
#include "daq.h"

constexpr uint8_t PIN_LED = 13;

using namespace daq;
FlexIODAQ DAQ{};

void setUp() {
  // set stuff up here
  bus::init();
}

void tearDown() {
  // clean stuff up here
}

void test_init() { TEST_ASSERT(DAQ.init(10'000)); }

void test_internals() {
  TEST_ASSERT_EQUAL(FlexIOHandler::flexIOHandler_list[1], DAQ.flexio);
  TEST_ASSERT_EQUAL(17, DAQ._flexio_pin_cnvst);
  TEST_ASSERT_EQUAL(10, DAQ._flexio_pin_clk);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_internals);
  DAQ.enable();
  UNITY_END();
}

void loop() {}