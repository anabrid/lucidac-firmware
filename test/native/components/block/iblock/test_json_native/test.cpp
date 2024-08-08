// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#define protected public
#define private public
#include "block/iblock.h"

blocks::IBlock iblock;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

auto serialized_data =
    R"({
		"outputs": { 1: 2 },
        "upscaling": { 2: true, 1: true }
        }
      })";

void json_to_object() {
  StaticJsonDocument<2048> doc;
  TEST_ASSERT(DeserializationError::Ok == deserializeJson(doc, serialized_data));
  std::cout << "Raw doc: " << doc << std::endl;

  JsonObject cfg = doc.as<JsonObject>();

  std::cout << "Object: " << cfg << std::endl;

  TEST_ASSERT_FALSE(cfg.isNull());

  TEST_ASSERT(iblock.config_self_from_json(cfg));
  TEST_ASSERT(iblock.get_upscaling(2));

  StaticJsonDocument<2048> newDoc;
  JsonObject newCfg = newDoc.to<JsonObject>();

  iblock.config_self_to_json(newCfg);

  std::cout << "New cfg: " << newCfg << std::endl;
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(json_to_object);
  UNITY_END();
}
