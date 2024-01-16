// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "daq/daq.h"
#include "lucidac.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace lucidac;
using namespace mode;

LUCIDAC luci{};
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

  // Put LUCIDAC start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(luci.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.m1block, "M1-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_trivial_single_connection() {
  u_int16_t delayTime = 1;
  luci.iblock->reset(true);
  luci.write_to_hardware();
  for (uint8_t outlane = 0; outlane < 16; outlane++) {
    for (uint8_t inlane = 0; inlane < 32; inlane++) {
      luci.iblock->connect(inlane,outlane, true);
      luci.write_to_hardware();
      
      char msgbuf[100];
      sprintf(msgbuf, "Input=%d", inlane);
      TEST_MESSAGE(msgbuf);

      delay(delayTime);
      luci.iblock->reset(true);
      luci.write_to_hardware();
      delay(delayTime);
    }
    delay(500);
  }

}

void test_single_output() {
  u_int16_t delayTime = 1;
  u_int64_t output = 0;
  luci.iblock->reset(true);
  luci.write_to_hardware();
  for (uint8_t inlane = 0; inlane < 32; inlane++) {
    luci.iblock->connect(inlane,output);
    luci.write_to_hardware();
    
    char msgbuf[100];
    sprintf(msgbuf, "Input=%d", inlane);
    TEST_MESSAGE(msgbuf);

    delay(delayTime);
    luci.iblock->reset(true);
    luci.write_to_hardware();
    delay(delayTime);
  }
  delay(500);
}

void test_single_input() {
  u_int16_t delayTime = 1;
  u_int64_t input = 31;
  luci.iblock->reset(true);
  luci.write_to_hardware();
  for (uint8_t outlane = 0; outlane < 16; outlane++) {
    luci.iblock->connect(input,outlane, true);
    luci.write_to_hardware();
    
    char msgbuf[100];
    sprintf(msgbuf, "Input=%d", outlane);
    TEST_MESSAGE(msgbuf);

    delay(delayTime);
    luci.iblock->reset(true);
    luci.write_to_hardware();
    delay(delayTime);
  }
  delay(500);
}

void test_single_connection() {
  u_int32_t delayTime = 100000;
  u_int8_t in1 = 20;
  u_int8_t out1 = 0;

  luci.iblock->reset(true);
  luci.write_to_hardware();

  luci.iblock->connect(in1,out1, true);
  luci.write_to_hardware();
    
  char msgbuf[100];
  sprintf(msgbuf, "In|Out = %d | %d", in1, out1);
  TEST_MESSAGE(msgbuf);
  delay(delayTime);
  
  luci.iblock->reset(true);
  luci.write_to_hardware();
  delay(10);
}

void test_double_connection() {
  u_int8_t in1 = 1;
  u_int8_t out1 = 0;  
  
  u_int8_t in2 = 2;
  u_int8_t out2 = 0;

  luci.iblock->reset(true);
  luci.write_to_hardware();

  luci.iblock->connect(in1,out1);
  luci.iblock->connect(in2,out2);
  luci.write_to_hardware();
    
  char msgbuf[100];
  sprintf(msgbuf, "%d --> %d  |  %d --> %d", in1, out1, in2, out2);
  TEST_MESSAGE(msgbuf);

  delay(100000);
  luci.iblock->reset(true);
  luci.write_to_hardware();
  delay(10);
}

void test_double_outputs() {
  u_int16_t delayTime = 1;

  u_int64_t out1 = 0;  
  u_int64_t out2 = 1;

  luci.iblock->reset(true);
  luci.write_to_hardware();

  for (uint8_t in1 = 0; in1 < 32; in1++) {
    for (uint8_t in2 = 0; in2 < 32; in2++) {
      luci.iblock->connect(in1,out1);
      luci.iblock->connect(in2,out2);
      luci.write_to_hardware();

      delay(delayTime);
      luci.iblock->reset(true);
      luci.write_to_hardware();
      delay(delayTime);
    }
    delay(500);
  }

}

void test_trivial_double_connection() {
  u_int16_t delayTime = 1;

  u_int16_t in1 = 0;
  u_int16_t out1 = 0;

  luci.iblock->reset(true);
  luci.write_to_hardware();
  for (uint8_t out2 = 0; out2 < 16; out2++) {
    for (uint8_t in2 = 0; in2 < 32; in2++) { 
      if (out1 == out2 && in1 == in2) continue;
      char msgbuf[100];
      sprintf(msgbuf, "Out2|In2 = %d | %d", out2, in2);
      TEST_MESSAGE(msgbuf);

      luci.iblock->connect(in2,out2);
      luci.iblock->connect(in1,out1);
      luci.write_to_hardware();
      delay(delayTime);

      luci.iblock->reset(true);
      luci.write_to_hardware();
      delay(delayTime);

      luci.iblock->connect(in2,out2);
      luci.write_to_hardware();
      delay(delayTime);

      luci.iblock->reset(true);
      luci.write_to_hardware();
      delay(delayTime);
    }
    delay(500);
  }

}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  //RUN_TEST(test_trivial_single_connection);
  RUN_TEST(test_single_connection);
  //RUN_TEST(test_single_output);
  //RUN_TEST(test_single_input);
  //RUN_TEST(test_single_connection);
  //RUN_TEST(test_trivial_double_connection);
  //RUN_TEST(test_double_connection);
  //RUN_TEST(test_double_outputs);
  UNITY_END();
}

void loop() {}
