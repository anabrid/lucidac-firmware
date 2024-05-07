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
#include "block/cblock.h"
#include "bus/functions.h"

blocks::CBlock cblockdonor;
blocks::CBlock cblockreciever;

void test_object2json2object() {
  cblockdonor.init();
  int num_coeff = cblockdonor.NUM_COEFF;
  float max_factor = cblockdonor.MAX_FACTOR;
  float min_factor = cblockdonor.MIN_FACTOR;

  for (int i = 0; i < num_coeff; i++) {
    float coeff_in_range = (max_factor * i + (num_coeff - i) * min_factor) / num_coeff;
    TEST_ASSERT(cblockdonor.set_factor(i, coeff_in_range));
    TEST_ASSERT_FLOAT_WITHIN(0.02, coeff_in_range, cblockdonor.get_factor(i));
  };

  TEST_ASSERT(cblockdonor.set_factor(0, 0.0));

  StaticJsonDocument<2048> doc;
  JsonObject test_cfg = doc.to<JsonObject>();
  cblockdonor.config_self_to_json(test_cfg);

  cblockreciever.init();
  cblockreciever.config_self_from_json(test_cfg);

  for (auto idx = 0; idx < cblockdonor.factors_.size(); idx++) {
    TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.02, cblockdonor.get_factor(idx), cblockreciever.get_factor(idx),
                                     std::to_string(idx).c_str());
  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_object2json2object);
  UNITY_END();
}

void loop() {}
