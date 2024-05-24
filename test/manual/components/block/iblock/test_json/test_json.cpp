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
blocks::IBlock iblockreciever;

void test_object2json2object() {
  iblockdonor.init();
  iblockdonor.reset_outputs();
  u_int8_t num_connections = iblockdonor.NUM_INPUTS;
  u_int8_t max_factor = iblockdonor.NUM_OUTPUTS;
  u_int8_t min_factor = 0;

  for (u_int8_t i = 0; i < num_connections; i++) {
    u_int8_t curr_connection = (max_factor * i + (num_connections - i) * min_factor) / num_connections;

    TEST_ASSERT(iblockdonor.connect(i, curr_connection, false, false));
    TEST_ASSERT(iblockdonor.is_connected(i, curr_connection));
  };

  StaticJsonDocument<2048> doc;
  JsonObject test_cfg = doc.to<JsonObject>();
  iblockdonor.config_self_to_json(test_cfg);

  iblockreciever.init();
  iblockreciever.config_self_from_json(test_cfg);

  for (auto idx = 0; idx < num_connections; idx++) {
    for (auto jdx = 0; jdx < num_connections; jdx++) {
      TEST_ASSERT_EQUAL(iblockdonor.is_connected(idx, jdx), iblockreciever.is_connected(idx, jdx));
    }
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_object2json2object);
  UNITY_END();
}

void loop() {}
