// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include <iostream>

#include "test_common.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"
#include "mode/mode.h"

#include "lucidac/lucidac.h"

#include "utils/running_avg.h"

bool extra_logs = false;
const float target_precision = 0.01f; // Replace with something from test_common.h

// This test case is setting the initial conditions of the integrators. This test runs isolated from the actual
// integration, so it can be used as a precursor to other integrator tests

using namespace platform;
using namespace blocks;
using namespace mode;

LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

void setUp() {
  // This is called before *each* test.
  carrier_.reset(true);
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

  carrier_.reset(false);
  TEST_ASSERT(carrier_.write_to_hardware());
}

bool error = false;
MIntBlock *int_block; // Workaround for unitys inability to have parametrised tests.

void setup_and_measure(bool use_slow_integration) {
  const float ic_values[] = {1.0f, 0.5f, 0.0f, -0.5f, -1.0f}; // Can be adjusted arbitrarily
  const int num_ic_values = sizeof(ic_values) / sizeof(float);

  // Static setup
  carrier_.reset(false);
  TEST_ASSERT(int_block->set_time_factors(use_slow_integration ? 100 : 10000));

  auto adc_channels = carrier_.get_adc_channels();
  for (unsigned int i = 0; i < adc_channels.size(); i++)
    adc_channels[i] = int_block->slot_to_global_io_index(i);

  TEST_ASSERT(carrier_.set_adc_channels(adc_channels));

  std::array<daq::data_vec_t, num_ic_values> measurements;

  // Dynamic IC values
  for (int i = 0; i < num_ic_values; i++) {
    TEST_ASSERT(int_block->set_ic_values(ic_values[i]));

    TEST_ASSERT(carrier_.write_to_hardware());
    TEST_ASSERT(carrier_.calibrate_routes(&DAQ));

    // Setup IC Values
    ManualControl::init();
    ManualControl::to_ic();
    delayMicroseconds(use_slow_integration ? 10'000 : 100);
    ManualControl::to_halt();

    // Measure outputs
    measurements[i] = DAQ.sample_avg(4, 10);
  }

  // Analyse results
  for (int ch = 0; ch < 8; ch++) {
    std::cout << "Channel " << ch << (use_slow_integration ? " slow" : " fast") << " mode ";
    bool local_error = false;
    for (int i = 0; i < num_ic_values; i++) {
      if (fabs(measurements[i][ch] + ic_values[i]) > target_precision) {
        error = true;
        local_error = true;
        std::cout << "failed at value: " << ic_values[i] << " measured: " << -measurements[i][ch] << "  ";
      }
    }
    if (!local_error)
      std::cout << "passed!" << std::endl;
    else
      std::cout << std::endl;
  }

  TEST_ASSERT_FALSE(error);
}

void test_all() {
  for (Cluster &cluster : carrier_.clusters) {
    for (MBlock *m_block : {cluster.m0block, cluster.m1block}) {
      if (!m_block || !m_block->is_entity_type(MBlock::TYPES::M_INT8_BLOCK))
        continue;

      int_block = static_cast<MIntBlock *>(m_block);

      std::cout << "Testing cluster " << +cluster.get_cluster_idx() << " slot " << int(m_block->slot) << ": "
                << std::endl;

      RUN_PARAM_TEST(setup_and_measure, false);
      RUN_PARAM_TEST(setup_and_measure, true);
    }
  }
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);
  if (extra_logs)
    msg::activate_serial_log();

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_all);
  UNITY_END();
}

void loop() {}
