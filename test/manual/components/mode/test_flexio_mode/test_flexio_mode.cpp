// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "test_parametrized.h"
#include <Arduino.h>
#include <SPI.h>
#include <unity.h>

#include "io/io.h"
#include "protocol/jsonl_logging.h"

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
  TEST_ASSERT_FALSE(
      FlexIOControl::init(99, mode::DEFAULT_OP_TIME, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE));
  TEST_ASSERT_FALSE(
      FlexIOControl::init(mode::DEFAULT_IC_TIME, 99, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE));
  // Too long
  TEST_ASSERT_FALSE(
      FlexIOControl::init(9'000'000'000, mode::DEFAULT_OP_TIME, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE));
  TEST_ASSERT_FALSE(FlexIOControl::init(mode::DEFAULT_IC_TIME, 9'000'000'000, mode::OnOverload::HALT,
                                        mode::OnExtHalt::IGNORE));
}

void test_simple_run() {
  TEST_ASSERT(
      FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE));
  delayMicroseconds(10);
  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
  }
  TEST_ASSERT_FALSE(FlexIOControl::is_overloaded());
}

void test_overload() {
  TEST_ASSERT(
      FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE));
  delayMicroseconds(10);
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
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::OnOverload::HALT,
                                  mode::OnExtHalt::PAUSE_THEN_RESTART));
  delayMicroseconds(10);
  FlexIOControl::force_start();

  // Trigger an external halt after some time in OP.
  // EXT HALT is currently implemented as a faulty pause state.
  // Meaning it moves back into OP once EXT HALT is inactive,
  // but it resets the OP timer and thus stays in OP longer.
  // TODO: Actually fixing this should just require changing FLEXIO_TIMCFG_TIMRST in TIMCFG of t_op timers.
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

void test_approximate_run_time(unsigned long long ic_time_ns, unsigned long long op_time_ns) {
  FlexIOControl::reset();
  TEST_ASSERT(FlexIOControl::init(ic_time_ns, op_time_ns, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE,
                                  Sync::MASTER));
  delayMicroseconds(10);

  auto t_start = micros();
  FlexIOControl::force_start();
  delayMicroseconds(1);
  while (!FlexIOControl::is_done()) {
  }
  auto t_end = micros();
  delayMicroseconds(1);
  TEST_ASSERT_FALSE(FlexIOControl::is_overloaded());
  TEST_ASSERT_FALSE(FlexIOControl::is_exthalt());

  if (t_end < t_start)
    TEST_FAIL_MESSAGE("micros timer overflow, just rerun :)");
  TEST_ASSERT_UINT_WITHIN(10, (ic_time_ns + op_time_ns) / 1000, t_end - t_start);
}

void test_sync_master() {
  //  !!!!!!!!!!!!
  //   !!      !!
  //    !!    !!
  //     !!  !!
  //      !!!!
  //
  //      !!!!
  //      !!!!
  //
  // WARNING: This test case expects a manual connection of SPI outputs to SYNC inputs
  //  SPI CLK pin 27 -> PIN_SYNC_CLK = 22
  //  SPI MOSI pin 26 -> PIN_SYNC_ID = 23

  SPI1.begin();
  SPI1.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));

  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::OnOverload::HALT,
                                  mode::OnExtHalt::IGNORE, mode::Sync::MASTER));
  delayMicroseconds(10);

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
  TEST_ASSERT(
      FlexIOControl::init(mode::DEFAULT_IC_TIME, 200'000, mode::OnOverload::HALT, mode::OnExtHalt::IGNORE));
}

void setup() {
  msg::activate_serial_log();
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
  RUN_PARAM_TEST(test_approximate_run_time, 1'000, 1'000'000);
  RUN_PARAM_TEST(test_approximate_run_time, 501'000, 500'000);
  RUN_PARAM_TEST(test_approximate_run_time, 601'000, 400'000);
  RUN_PARAM_TEST(test_approximate_run_time, 701'000, 300'000);
  RUN_PARAM_TEST(test_approximate_run_time, 801'000, 200'000);
  RUN_PARAM_TEST(test_approximate_run_time, 901'000, 100'000);
  RUN_PARAM_TEST(test_approximate_run_time, 911'000, 90'000);
  RUN_PARAM_TEST(test_approximate_run_time, 921'000, 80'000);
  RUN_PARAM_TEST(test_approximate_run_time, 931'000, 70'000);
  RUN_PARAM_TEST(test_approximate_run_time, 941'000, 60'000);
  RUN_PARAM_TEST(test_approximate_run_time, 951'000, 50'000);
  RUN_PARAM_TEST(test_approximate_run_time, 961'000, 40'000);
  RUN_PARAM_TEST(test_approximate_run_time, 971'000, 31'000);
  RUN_PARAM_TEST(test_approximate_run_time, 981'000, 20'000);
  RUN_PARAM_TEST(test_approximate_run_time, 991'000, 10'000);
  RUN_PARAM_TEST(test_approximate_run_time, 992'000, 9'000);
  RUN_PARAM_TEST(test_approximate_run_time, 993'000, 8'000);
  RUN_PARAM_TEST(test_approximate_run_time, 994'000, 7'000);
  RUN_PARAM_TEST(test_approximate_run_time, 995'000, 6'000);
  RUN_PARAM_TEST(test_approximate_run_time, 996'000, 5'000);
  RUN_PARAM_TEST(test_approximate_run_time, 997'000, 4'000);
  RUN_PARAM_TEST(test_approximate_run_time, 998'000, 3'000);
  RUN_PARAM_TEST(test_approximate_run_time, 999'000, 2'000);
  RUN_PARAM_TEST(test_approximate_run_time, 1'000'000, 1'000);
  RUN_PARAM_TEST(test_approximate_run_time, 2'000, 10'000);
  RUN_PARAM_TEST(test_approximate_run_time, 5'000, 10'000);
  RUN_PARAM_TEST(test_approximate_run_time, 50'000, 10'000);
  RUN_PARAM_TEST(test_approximate_run_time, 100'000, 10'000);
  RUN_PARAM_TEST(test_approximate_run_time, 200'000, 10'000);
  RUN_PARAM_TEST(test_approximate_run_time, 300'000, 10'000);
  RUN_TEST(test_exthalt);
  RUN_TEST(test_overload);
  // RUN_TEST(test_sync_master);
  // RUN_TEST(test_sync_slave);
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
