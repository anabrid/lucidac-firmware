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

bool extra_logs = true;
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

  TEST_ASSERT(carrier_.write_to_hardware());
}

void setup_and_measure() {
  // Setup paths
  auto int_block = static_cast<MIntBlock *>(cluster.m0block);

  std::cout << carrier_.calibrate_m_blocks(&DAQ) << std::endl;

  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 0, 0.5f, 0)); // This creates y = -1/2*x
  TEST_ASSERT(cluster.route(0, 24, 1.0f, 2));
  TEST_ASSERT(cluster.route_out_external(2, 1, 1.0f)); // measurement of int parabola

  TEST_ASSERT(cluster.route(0, 26, 1.0f, 8)); // Mul x0
  TEST_ASSERT(cluster.route(0, 27, 1.0f, 9)); // Mul y0

  TEST_ASSERT(cluster.route_out_external(8, 4, 1.0f)); // measurement of mul parabola

  TEST_ASSERT(int_block->set_time_factors(10000));
  TEST_ASSERT(int_block->set_ic_value(0, -1.0f));
  TEST_ASSERT(int_block->set_ic_value(2, -1.0f));

  TEST_ASSERT(carrier_.write_to_hardware());
  TEST_ASSERT(carrier_.calibrate_routes(&DAQ));

  // Measure end value
  TEST_ASSERT(
      FlexIOControl::init(mode::DEFAULT_IC_TIME, 400'000, mode::OnOverload::IGNORE, mode::OnExtHalt::IGNORE));
  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
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
  RUN_TEST(setup_and_measure);
  UNITY_END();
}

void loop() {}
