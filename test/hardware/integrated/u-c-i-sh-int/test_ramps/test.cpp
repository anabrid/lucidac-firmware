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

// This test case is doing some simple ramp test cases, so it's integrating a constant. This is the easiest
// automatic test you can do, to test the full functionality of the interators.

using namespace platform;
using namespace blocks;
using namespace mode;

LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

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
Cluster *cluster; // Workaround for unitys inability to have parametrised tests.
MIntBlock *int_block;

void setup_and_measure(bool use_slow_integration) {
  // Setup paths
  carrier_.reset(false);
  for (uint8_t int_channel : MIntBlock::INTEGRATORS_INPUT_RANGE())
    TEST_ASSERT(cluster->add_constant(UBlock::Transmission_Mode::POS_REF,
                                      int_block->slot_to_global_io_index(int_channel), 1.0f,
                                      int_block->slot_to_global_io_index(int_channel)));

  TEST_ASSERT(int_block->set_time_factors(use_slow_integration ? 100 : 10000));
  TEST_ASSERT(int_block->set_ic_values(0.0f));

  auto adc_channels = carrier_.get_adc_channels();
  for (unsigned int i = 0; i < adc_channels.size(); i++)
    adc_channels[i] = i;

  TEST_ASSERT(carrier_.set_adc_channels(adc_channels));

  TEST_ASSERT(carrier_.write_to_hardware());
  TEST_ASSERT(carrier_.calibrate_routes(&DAQ));

  // Measure end value
  TEST_ASSERT(FlexIOControl::init(use_slow_integration ? mode::DEFAULT_IC_TIME * 100 : mode::DEFAULT_IC_TIME,
                                  use_slow_integration ? 10'000'000 : 100'000, mode::OnOverload::IGNORE,
                                  mode::OnExtHalt::IGNORE));

  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
  }

  auto readings = DAQ.sample_avg(4, 10);

  for (int i = 0; i < 8; i++) {
    if (fabs(readings[i] + 1.0f) > target_precision) {
      error = true;
      std::cout << "Channel " << i << (use_slow_integration ? " slow" : " fast")
                << " mode failed!  Read value: " << readings[i] << std::endl;
    } else
      std::cout << "Channel " << i << (use_slow_integration ? " slow" : " fast")
                << " mode passed!  Read value: " << readings[i] << std::endl;
  }
  TEST_ASSERT_FALSE(error);
}

void test_all() {
  for (Cluster &cluster_it : carrier_.clusters) {
    for (MBlock *m_block : {cluster_it.m0block, cluster_it.m1block}) {
      if (!m_block || !m_block->is_entity_type(MBlock::TYPES::M_INT8_BLOCK))
        continue;

      cluster = &cluster_it;
      int_block = static_cast<MIntBlock *>(m_block);

      std::cout << "Testing cluster " << +cluster_it.get_cluster_idx() << " slot " << int(m_block->slot)
                << ": " << std::endl;

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
