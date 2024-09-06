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

// This test case is doing some double integration of a constant, which yields a parabola. Currently there
// isn't much variety or automation.
// You can also test an multiplication block with this testcase without any changes to the code.

using namespace platform;
using namespace blocks;
using namespace mode;

LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

auto &cluster = carrier_.clusters[0];

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
    TEST_ASSERT_NOT_NULL(cluster.m0block);
    TEST_ASSERT(cluster.m0block->is_entity_type(MBlock::TYPES::M_INT8_BLOCK));
  }
  TEST_ASSERT_NOT_NULL(carrier_.ctrl_block);

  carrier_.reset(false);
  TEST_ASSERT(carrier_.write_to_hardware());
}

void setup_and_measure() {
  // Setup paths
  carrier_.ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());
  cluster.reset(false);

  auto int_block = static_cast<MIntBlock *>(cluster.m0block);

  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 0, 0.5f, 0)); // This creates y = -1/2*x
  TEST_ASSERT(cluster.route(0, 24, 1.0f, 2));
  TEST_ASSERT(cluster.route(2, 25, 1.0f, 1)); // just for measurement of int block

  TEST_ASSERT(cluster.route(0, 26, 1.0f, 8)); // Mul x0
  TEST_ASSERT(cluster.route(0, 27, 1.0f, 9)); // Mul y0

  TEST_ASSERT(cluster.route(8, 28, 1.0f, 3)); // just for measurement of mul block

  TEST_ASSERT(int_block->set_time_factors(10000));
  TEST_ASSERT(int_block->set_ic_value(0, -1.0f));
  TEST_ASSERT(int_block->set_ic_value(2, -1.0f));

  TEST_ASSERT(carrier_.write_to_hardware());

  TEST_ASSERT(cluster.calibrate_routes(&DAQ));

  carrier_.ctrl_block->set_adc_bus(CTRLBlock::ADCBus::ADC);

  auto adc_channels = carrier_.get_adc_channels();
  for (unsigned int i = 0; i < adc_channels.size(); i++)
    adc_channels[i] = i; // identity connection only works for m0

  TEST_ASSERT(carrier_.set_adc_channels(adc_channels));

  TEST_ASSERT(carrier_.write_to_hardware());

  // Measure end value
  TEST_ASSERT(
      FlexIOControl::init(mode::DEFAULT_IC_TIME, 4'00'000, mode::OnOverload::IGNORE, mode::OnExtHalt::IGNORE));
  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
  }

  auto readings = DAQ.sample_avg(4, 10);
  std::cout << readings << std::endl;
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);
  if (extra_logs)
    msg::activate_serial_log();

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(setup_and_measure);
  UNITY_END();
}

void loop() {}
