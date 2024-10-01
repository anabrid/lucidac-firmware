// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/*
 * NOTES
 * - FlexIODAQ can not be instantiated in top-level code outside of functions
 */

#include "unity.h"
#include <Arduino.h>

#include "cmath"

#include "../test_waveforms_utils.h"
#include "test_parametrized.h"

// Include things with all private members made public,
// to inspect internal variables without additional hassle.
#define private public
#define protected public
#include "daq/daq.h"
#include "lucidac/lucidac.h"
#include "mode/mode.h"
#include "run/run_manager.h"

using namespace blocks;
using namespace daq;
using namespace mode;
using namespace run;

LUCIDAC carrier_;

// std::vector<const char *> DATA;
//  std::vector<uint16_t> DATA;

Run configure_sinusoidal() {
  // We do need certain blocks
  // TODO: Make independent of configuration
  TEST_ASSERT_EQUAL_MESSAGE(1, carrier_.clusters.size(), "This test currently only works with one cluster.");
  for (auto &cluster : carrier_.clusters) {
    TEST_ASSERT_NOT_NULL_MESSAGE(cluster.m0block, "This test currently requires an M-INT-block in M0.");
    TEST_ASSERT_MESSAGE(cluster.m0block->is_entity_type(blocks::MBlock::TYPES::M_INT8_BLOCK),
                        "This test currently requires an M-INT-block in M0.");
  }

  // Configure some sinusoidal waveforms
  for (auto &cluster : carrier_.clusters) {
    for (auto i : {0u, 1u, 2u, 3u}) {
      TEST_ASSERT(cluster.route(i * 2 + 0, i * 2 + 0, -1.0f, i * 2 + 1));
      TEST_ASSERT(cluster.route(i * 2 + 1, i * 2 + 1, 1.0f, i * 2 + 0));
      TEST_ASSERT(static_cast<MIntBlock *>(cluster.m0block)->set_ic_value(i * 2 + 0, 0.0f));
      TEST_ASSERT(static_cast<MIntBlock *>(cluster.m0block)->set_ic_value(i * 2 + 1, 0.5f));
    }
  }
  TEST_ASSERT(carrier_.set_adc_channels({0, 1, 2, 3, 4, 5, 6, 7}));
  TEST_ASSERT(carrier_.write_to_hardware());

  // Run partial calibration
  TEST_ASSERT(carrier_.calibrate_offset());

  return {
      "", {mode::DEFAULT_IC_TIME, static_cast<unsigned long long>(2 * PI * 100'000 + 2'000)}, {8, 100'000}};
}

bool check_sinusoidal(const Run &run, const std::vector<float> &data) {
  if (run.daq_config.get_num_channels() != 8 or data.size() % 8) {
    TEST_FAIL_MESSAGE("Unexpected number of channels or data size.");
    return false;
  }

  // Compare with expected mathematical function
  for (auto idx = 0u; idx < data.size(); idx += 8) {
    float t = run.daq_config.index_to_time(idx / 8);
    auto sin_expected = 0.5f * std::sin(10000 * t);
    auto cos_expected = -0.5f * std::cos(10000 * t);
    for (auto i : {0u, 1u, 2u, 3u}) {
      TEST_ASSERT_FLOAT_WITHIN(0.02f, sin_expected, data[idx + (i * 2 + 0)]);
      TEST_ASSERT_FLOAT_WITHIN(0.02f, cos_expected, data[idx + (i * 2 + 1)]);
    }
  }
  return true;
}

void tearDown() {
  // This is called after *each* test.
}

void setup() {
  bus::init();
  msg::activate_serial_log();

  UNITY_BEGIN();
  RUN_PARAM_TEST(test_waveform, carrier_, configure_sinusoidal, check_sinusoidal);
  UNITY_END();
}

void loop() {}
