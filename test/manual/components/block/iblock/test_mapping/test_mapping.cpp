// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>
#include <math.h>
#include <unity.h>

#define private public
#define protected public
#include "block/iblock.h"
#include "bus/functions.h"

blocks::IBlock iblockdonor;

void test_idx2hw2idx() {
  iblockdonor.init();
  iblockdonor.reset_outputs();
  u_int8_t num_connections = iblockdonor.NUM_INPUTS;

  for (u_int8_t i = 0; i < num_connections; i++) {

    TEST_ASSERT_EQUAL(i, iblockdonor._hardware_to_logical_input(iblockdonor._logical_to_hardware_input(i)));
  };
  for (u_int8_t i = 0; i < num_connections; i++) {

    TEST_ASSERT_EQUAL(i, iblockdonor._logical_to_hardware_input(iblockdonor._hardware_to_logical_input(i)));
  };
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_idx2hw2idx);
  UNITY_END();
}

void loop() {}
