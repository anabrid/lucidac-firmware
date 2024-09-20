// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <sstream>
#include <unity.h>

#include "daq/daq.h"
#include "io/io.h"
#include "mode/mode.h"
#include "protocol/jsonl_logging.h"

#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;
using namespace carrier;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

// Parametrization for test cases
static std::array<int8_t, 8> adc_channels{Carrier::ADC_CHANNEL_DISABLED, Carrier::ADC_CHANNEL_DISABLED,
                                          Carrier::ADC_CHANNEL_DISABLED, Carrier::ADC_CHANNEL_DISABLED,
                                          Carrier::ADC_CHANNEL_DISABLED, Carrier::ADC_CHANNEL_DISABLED,
                                          Carrier::ADC_CHANNEL_DISABLED, Carrier::ADC_CHANNEL_DISABLED};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init_and_blocks() {
  // In carrier_.init(), missing blocks are ignored
  TEST_ASSERT(carrier_.init());
  // We do need certain blocks
  for (auto &cluster : carrier_.clusters) {
    TEST_ASSERT_NOT_NULL(cluster.ublock);
    TEST_ASSERT_NOT_NULL(cluster.cblock);
    TEST_ASSERT_NOT_NULL(cluster.iblock);
    TEST_ASSERT_NOT_NULL(cluster.shblock);
  }
  TEST_ASSERT_NOT_NULL(carrier_.ctrl_block);
  // Reset
  TEST_ASSERT(carrier_.write_to_hardware());

  // Route constants over all lanes
  for (auto &cluster : carrier_.clusters)
    for (auto lane_idx : IBlock::OUTPUT_IDX_RANGE()) {
      TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 2 * lane_idx, 1.0f, lane_idx));
      TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 2 * lane_idx + 1, 1.0f, lane_idx));
    }
  TEST_ASSERT(carrier_.write_to_hardware());
}

void test_interference() {
  // Set adc channels
  TEST_ASSERT(carrier_.set_adc_channels(adc_channels));
  TEST_ASSERT(carrier_.write_to_hardware());
  // Run calibration
  std::stringstream message;
  message << adc_channels;
  TEST_ASSERT_MESSAGE(carrier_.calibrate_routes(&DAQ), message.str().c_str());
}

void setup() {
  bus::init();
  io::init();
  // Since integrators are involved, initialize to halt mode
  mode::ManualControl::init();
  DAQ.init(0);
  msg::activate_serial_log();

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);

  std::fill(adc_channels.begin(), adc_channels.end(), Carrier::ADC_CHANNEL_DISABLED);
  RUN_TEST(test_interference);

  adc_channels = {0, 1, 2, 3, 4, 5, 6, 7};
  RUN_TEST(test_interference);

  adc_channels = {8, 9, 10, 11, 12, 13, 14, 15};
  RUN_TEST(test_interference);

  for (auto in_idx : {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15})
    for (auto out_idx : {0, 1, 2, 3, 4, 5, 6, 7}) {
      std::fill(adc_channels.begin(), adc_channels.end(), Carrier::ADC_CHANNEL_DISABLED);
      adc_channels[out_idx] = static_cast<char>(in_idx);

      RUN_TEST(test_interference);
    }

  // Check that all disabled still work
  std::fill(adc_channels.begin(), adc_channels.end(), Carrier::ADC_CHANNEL_DISABLED);
  RUN_TEST(test_interference);

  UNITY_END();
}

void loop() { delay(500); }
