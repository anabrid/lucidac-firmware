// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#define private public
#define protected public

#include "block/ublock.h"

// U Block is just for example
blocks::UBlock ublock;

auto serialized_data =
    R"({
        "outputs": { 0: 2 },
        "nonsense": "not valid"
      })";

void json_to_object() {
  StaticJsonDocument<2048> doc;
  TEST_ASSERT(DeserializationError::Ok == deserializeJson(doc, serialized_data));
  std::cout << "Raw doc: " << doc << std::endl;

  JsonObject cfg = doc.as<JsonObject>();

  std::cout << "Object: " << cfg << std::endl;

  TEST_ASSERT_FALSE(cfg.isNull());

  TEST_ASSERT_FALSE(ublock.config_self_from_json(cfg));
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(json_to_object);
  UNITY_END();
}
