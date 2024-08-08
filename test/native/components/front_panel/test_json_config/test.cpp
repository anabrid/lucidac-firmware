// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#define private public
#include "lucidac/front_panel.h"

platform::LUCIDACFrontPanel fp;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

auto serialized_data =
    R"({
        "leds": 1,
        "signal_generator": {
          "frequency": 1000.0,
          "phase": 0.0,
          "wave_form": "sine_and_square",
          "amplitude": 1.0,
          "square_voltage_low": -1.0,
          "square_voltage_high": 1.0,
          "offset": 0.0,
          "sleep": false,

          "dac_outputs": [1.5, -1.0]
        }
      })";

void json_to_object() {
  StaticJsonDocument<2048> doc;
  TEST_ASSERT(DeserializationError::Ok == deserializeJson(doc, serialized_data));
  std::cout << "Raw doc: " << doc << std::endl;

  JsonObject cfg = doc.as<JsonObject>();

  std::cout << "Object: " << cfg << std::endl;

  TEST_ASSERT_FALSE(cfg.isNull());

  TEST_ASSERT(fp.config_self_from_json(cfg));

  TEST_ASSERT_EQUAL(1, fp.leds.get_all());
  TEST_ASSERT_EQUAL(1000.0f, fp.signal_generator.get_frequency());
  TEST_ASSERT_EQUAL(0.0f, fp.signal_generator.get_phase());
  TEST_ASSERT_EQUAL(functions::AD9834::WaveForm::SINE_AND_SQUARE, fp.signal_generator.get_wave_form());
  TEST_ASSERT_EQUAL(1.0f, fp.signal_generator.get_amplitude());
  TEST_ASSERT_EQUAL(-1.0f, fp.signal_generator.get_square_voltage_low());
  TEST_ASSERT_EQUAL(1.0f, fp.signal_generator.get_square_voltage_high());
  TEST_ASSERT_EQUAL(0.0f, fp.signal_generator.get_offset());
  TEST_ASSERT_EQUAL(false, fp.signal_generator.get_sleep());
  TEST_ASSERT_EQUAL(1.5f, fp.signal_generator.get_dac_out0());
  TEST_ASSERT_EQUAL(-1.0f, fp.signal_generator.get_dac_out1());

  StaticJsonDocument<2048> newDoc;
  JsonObject newCfg = newDoc.to<JsonObject>();

  fp.config_self_to_json(newCfg);

  std::cout << "New cfg: " << newCfg << std::endl;

  TEST_ASSERT_EQUAL(cfg, newCfg);
}

int main(int argc, char **argv) {
  UNITY_BEGIN();
  RUN_TEST(json_to_object);
  UNITY_END();
}
