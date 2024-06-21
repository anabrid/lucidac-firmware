// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <SPI.h>
#include <unity.h>

#include "io/io.h"

#define protected public
#define private public
#include "mode/mode.h"

//  !!!!!!!!!!!!
//   !!      !!
//    !!    !!
//     !!  !!
//      !!!!
//
//      !!!!
//      !!!!
//
// WARNING: Some test cases need manual connections to fake overload & exthalt signals!
// Connect the following pins manually to PIN_MODE_OVERLOAD and PIN_MODE_EXTHALT!
// Also connect SPI_CLK and MOSI to SYNC_CLK and SYNC_ID
constexpr uint8_t FAKE_OVERLOAD_PIN = io::PIN_DIO_11;
constexpr uint8_t FAKE_EXTHALT_PIN = io::PIN_DIO_12;

using namespace mode;

void setUp() {
  // This is called before *each* test.
  FlexIOControl::reset();
  // Use io::PIN_RESERVED_7 as trigger for oscilloscope
  // Use test-flexio-mode saved configuration for easy testing
  digitalWriteFast(io::PIN_RESERVED_7, HIGH);
}

void tearDown() {
  // This is called after *each* test.
  digitalWriteFast(io::PIN_RESERVED_7, LOW);
}

void test_limits() {
  // Too short
  TEST_ASSERT_FALSE(FlexIOControl::init(99, mode::DEFAULT_OP_TIME));
  TEST_ASSERT_FALSE(FlexIOControl::init(mode::DEFAULT_IC_TIME, 99));
  // Too long
  TEST_ASSERT_FALSE(FlexIOControl::init(275'000, mode::DEFAULT_OP_TIME));
  TEST_ASSERT_FALSE(FlexIOControl::init(mode::DEFAULT_IC_TIME, 9'000'000'000));
}

void test_simple_run() {
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000));
  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
  }
  TEST_ASSERT_FALSE(FlexIOControl::is_overloaded());
}

void test_overload() {
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000));
  FlexIOControl::force_start();

  // Trigger a fake overload after some time in OP
  delayMicroseconds(200);
  digitalWriteFast(FAKE_OVERLOAD_PIN, LOW);
  delayMicroseconds(10);
  digitalWriteFast(FAKE_OVERLOAD_PIN, HIGH);

  // We should end up in overload state immediately
  delayMicroseconds(1);
  TEST_ASSERT(FlexIOControl::is_done());
  TEST_ASSERT(FlexIOControl::is_overloaded());
}

void test_exthalt() {
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000));
  FlexIOControl::force_start();

  // Trigger an external halt after some time in OP.
  // EXT HALT is currently implemented as a faulty pause state.
  // Meaning it moves back into OP once EXT HALT is inactive,
  // but it resets the OP timer and thus stays in OP longer.
  delayMicroseconds(200);
  digitalWriteFast(FAKE_EXTHALT_PIN, LOW);
  delayMicroseconds(1);
  TEST_ASSERT(FlexIOControl::is_exthalt());
  delayMicroseconds(10);
  digitalWriteFast(FAKE_EXTHALT_PIN, HIGH);

  // After EXT HALT, we should be in OP for the full op time (see above)
  delayMicroseconds(1);
  TEST_ASSERT_FALSE(FlexIOControl::is_done());

  delayMicroseconds(200 + 5);
  TEST_ASSERT(FlexIOControl::is_done());
  TEST_ASSERT_FALSE(FlexIOControl::is_overloaded());
}

void test_approximate_run_time() {
  for (auto op_time_ns :
       {1'000'000ull, 500'000ull, 400'000ull, 300'000ull, 200'000ull, 100'000ull, 90'000ull, 80'000ull,
        70'000ull,    60'000ull,  50'000ull,  40'000ull,  30'000ull,  20'000ull,  10'000ull, 9'000ull,
        8'000ull,     7'000ull,   6'000ull,   5'000ull,   4'000ull,   3'000ull,   2'000ull,  1'000ull}) {
    FlexIOControl::reset();
    TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, op_time_ns));
    delayMicroseconds(1);

    auto t_start = micros();
    FlexIOControl::force_start();
    while (!FlexIOControl::is_done()) {
    }
    auto t_end = micros();
    delayMicroseconds(1);
    TEST_ASSERT_FALSE(FlexIOControl::is_overloaded());
    TEST_ASSERT_FALSE(FlexIOControl::is_exthalt());

    if (t_end < t_start)
      TEST_FAIL_MESSAGE("micros timer overflow, just rerun :)");
    TEST_ASSERT_UINT_WITHIN(10, (mode::DEFAULT_IC_TIME + op_time_ns) / 1000, t_end - t_start);
  }
}

void test_sync_master() {
  SPI1.begin();
  SPI1.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));

  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::Sync::MASTER));

  TEST_MESSAGE("Press button to trigger state machine.");
  io::block_until_button_press();

  // Send 16 zeros, which should trigger nothing
  SPI1.transfer16(0b00000000'00000000);
  delayMicroseconds(10);
  TEST_ASSERT(FlexIOControl::is_idle());

  // Send a bunch of matching data, which should trigger
  SPI1.transfer16(0b11110000'10101010);
  delayMicroseconds(10);
  TEST_ASSERT_FALSE(FlexIOControl::is_idle());

  // Check if we are going into OP and end after op time
  delayMicroseconds(100);
  TEST_ASSERT(FlexIOControl::is_op());
  delayMicroseconds(200);
  TEST_ASSERT(FlexIOControl::is_done());
}

void test_sync_slave() {
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::Sync::SLAVE));
}

void setup() {
  io::init();

  // We use some pins as test/trigger signals in this test
  // Overwrite their pin mode
  pinMode(io::PIN_RESERVED_7, OUTPUT);
  pinMode(FAKE_OVERLOAD_PIN, OUTPUT);
  digitalWriteFast(FAKE_OVERLOAD_PIN, HIGH);
  pinMode(FAKE_EXTHALT_PIN, OUTPUT);
  digitalWriteFast(FAKE_EXTHALT_PIN, HIGH);

  UNITY_BEGIN();
  RUN_TEST(test_limits);
  RUN_TEST(test_simple_run);
  RUN_TEST(test_approximate_run_time);
  RUN_TEST(test_exthalt);
  RUN_TEST(test_overload);
  RUN_TEST(test_sync_master);
  //RUN_TEST(test_sync_slave);
  UNITY_END();
}

void loop() {
  /*
  static auto op_time = mode::DEFAULT_OP_TIME;
  delay(max(op_time/1'000'000, 10000));
  if (FlexIOControl::init(mode::DEFAULT_IC_TIME, op_time)) {
    FlexIOControl::force_start();
    op_time *= 2;
  } else {
    op_time = mode::DEFAULT_OP_TIME;
  }
  */
}
