// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/carrier.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace platform;
using namespace mode;

Cluster cluster{};
OneshotDAQ daq_{};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Initialize mode controller (currently separate thing)
  ManualControl::init();

  // Put cluster start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(cluster.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.m0block, "M0-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(cluster.calibrate_routes(&daq_));
  delayMicroseconds(200);
}

void test_trivial_single_connection() {
  u_int16_t delayTime = 1;
  cluster.iblock->reset(true);
  cluster.write_to_hardware();
  for (uint8_t outlane = 0; outlane < 16; outlane++) {
    for (uint8_t inlane = 0; inlane < 32; inlane++) {
      cluster.iblock->connect(inlane, outlane, true);
      cluster.write_to_hardware();

      char msgbuf[100];
      sprintf(msgbuf, "Input=%d", inlane);
      TEST_MESSAGE(msgbuf);

      delay(delayTime);
      cluster.iblock->reset(true);
      cluster.write_to_hardware();
      delay(delayTime);
    }
    delay(500);
  }
}

void test_single_output() {
  u_int16_t delayTime = 1;
  u_int64_t output = 0;
  cluster.iblock->reset(true);
  cluster.write_to_hardware();
  for (uint8_t inlane = 0; inlane < 32; inlane++) {
    cluster.iblock->connect(inlane, output);
    cluster.write_to_hardware();

    char msgbuf[100];
    sprintf(msgbuf, "Input=%d", inlane);
    TEST_MESSAGE(msgbuf);

    delay(delayTime);
    cluster.iblock->reset(true);
    cluster.write_to_hardware();
    delay(delayTime);
  }
  delay(500);
}

void test_single_input() {
  u_int16_t delayTime = 1;
  u_int64_t input = 31;
  cluster.iblock->reset(true);
  cluster.write_to_hardware();
  for (uint8_t outlane = 0; outlane < 16; outlane++) {
    cluster.iblock->connect(input, outlane, true);
    cluster.write_to_hardware();

    char msgbuf[100];
    sprintf(msgbuf, "Input=%d", outlane);
    TEST_MESSAGE(msgbuf);

    delay(delayTime);
    cluster.iblock->reset(true);
    cluster.write_to_hardware();
    delay(delayTime);
  }
  delay(500);
}

void test_single_connection() {
  u_int32_t delayTime = 100000;
  u_int8_t in1 = 20;
  u_int8_t out1 = 0;

  cluster.iblock->reset(true);
  cluster.write_to_hardware();

  cluster.iblock->connect(in1, out1, true);
  cluster.write_to_hardware();

  char msgbuf[100];
  sprintf(msgbuf, "In|Out = %d | %d", in1, out1);
  TEST_MESSAGE(msgbuf);
  delay(delayTime);

  cluster.iblock->reset(true);
  cluster.write_to_hardware();
  delay(10);
}

void test_double_connection() {
  u_int8_t in1 = 1;
  u_int8_t out1 = 0;

  u_int8_t in2 = 2;
  u_int8_t out2 = 0;

  cluster.iblock->reset(true);
  cluster.write_to_hardware();

  cluster.iblock->connect(in1, out1);
  cluster.iblock->connect(in2, out2);
  cluster.write_to_hardware();

  char msgbuf[100];
  sprintf(msgbuf, "%d --> %d  |  %d --> %d", in1, out1, in2, out2);
  TEST_MESSAGE(msgbuf);

  delay(100000);
  cluster.iblock->reset(true);
  cluster.write_to_hardware();
  delay(10);
}

void test_double_outputs() {
  u_int16_t delayTime = 1;

  u_int64_t out1 = 0;
  u_int64_t out2 = 1;

  cluster.iblock->reset(true);
  cluster.write_to_hardware();

  for (uint8_t in1 = 0; in1 < 32; in1++) {
    for (uint8_t in2 = 0; in2 < 32; in2++) {
      cluster.iblock->connect(in1, out1);
      cluster.iblock->connect(in2, out2);
      cluster.write_to_hardware();

      delay(delayTime);
      cluster.iblock->reset(true);
      cluster.write_to_hardware();
      delay(delayTime);
    }
    delay(500);
  }
}

void test_trivial_double_connection() {
  u_int16_t delayTime = 1;

  u_int16_t in1 = 0;
  u_int16_t out1 = 0;

  cluster.iblock->reset(true);
  cluster.write_to_hardware();
  for (uint8_t out2 = 0; out2 < 16; out2++) {
    for (uint8_t in2 = 0; in2 < 32; in2++) {
      if (out1 == out2 && in1 == in2)
        continue;
      char msgbuf[100];
      sprintf(msgbuf, "Out2|In2 = %d | %d", out2, in2);
      TEST_MESSAGE(msgbuf);

      cluster.iblock->connect(in2, out2);
      cluster.iblock->connect(in1, out1);
      cluster.write_to_hardware();
      delay(delayTime);

      cluster.iblock->reset(true);
      cluster.write_to_hardware();
      delay(delayTime);

      cluster.iblock->connect(in2, out2);
      cluster.write_to_hardware();
      delay(delayTime);

      cluster.iblock->reset(true);
      cluster.write_to_hardware();
      delay(delayTime);
    }
    delay(500);
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  // RUN_TEST(test_trivial_single_connection);
  RUN_TEST(test_single_connection);
  // RUN_TEST(test_single_output);
  // RUN_TEST(test_single_input);
  // RUN_TEST(test_single_connection);
  // RUN_TEST(test_trivial_double_connection);
  // RUN_TEST(test_double_connection);
  // RUN_TEST(test_double_outputs);
  UNITY_END();
}

void loop() {}
