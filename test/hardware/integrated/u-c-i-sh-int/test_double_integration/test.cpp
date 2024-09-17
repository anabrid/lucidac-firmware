// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include <iostream>

#define protected public
#define private public

#include "test_common.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"
#include "mode/mode.h"

#include "lucidac/lucidac.h"

#include "utils/running_avg.h"

bool extra_logs = false;
const float target_precision = 0.01f; // Replace with something from test_common.h

using namespace platform;
using namespace blocks;
using namespace mode;

LUCIDAC lucidac;
daq::OneshotDAQ DAQ;

auto &cluster = lucidac.clusters[0];

void setUp() {
  // This is called before *each* test.
  lucidac.reset(true);
}

void tearDown() {
  // This is called after *each* test.
}

void test_init_and_blocks() {
  // In lucidac.init(), missing blocks are ignored
  TEST_ASSERT(lucidac.init());
  // We do need certain blocks
  for (auto &cluster : lucidac.clusters) {
    TEST_ASSERT_NOT_NULL(cluster.ublock);
    TEST_ASSERT_NOT_NULL(cluster.cblock);
    TEST_ASSERT_NOT_NULL(cluster.iblock);
    TEST_ASSERT_NOT_NULL(cluster.shblock);
    TEST_ASSERT_NOT_NULL(cluster.m0block);
    TEST_ASSERT(cluster.m0block->is_entity_type(MBlock::TYPES::M_INT8_BLOCK));
    TEST_ASSERT_NOT_NULL(cluster.m1block);
    TEST_ASSERT(cluster.m1block->is_entity_type(MBlock::TYPES::M_MUL4_BLOCK));
  }
  TEST_ASSERT_NOT_NULL(lucidac.ctrl_block);
  TEST_ASSERT_NOT_NULL(lucidac.front_panel);
  TEST_ASSERT(lucidac.front_panel->signal_generator.is_installed());

  lucidac.reset(false);
  TEST_ASSERT(lucidac.write_to_hardware());
}

void test() {
  cluster.reset(false);

  auto int_block = static_cast<MIntBlock *>(cluster.m0block);

  auto &signal_generator = lucidac.front_panel->signal_generator;
  signal_generator.set_frequency(10000.0f /
                                 (2.0f * 3.14f)); // create cosine with omega = 1 on integrators time scale
  signal_generator.set_phase(3.14f / 2.0f);
  signal_generator.set_amplitude(1.0f);
  signal_generator.set_offset(0.00375f);
  signal_generator.awake();

  TEST_ASSERT(int_block->set_time_factors(10000));
  TEST_ASSERT(int_block->set_ic_value(2, 1.25f / 2.0f));

  // Setup actual circuit for calulation
  TEST_ASSERT(lucidac.set_acl_select(0, LUCIDAC::ACL::EXTERNAL_));
  TEST_ASSERT(cluster.route_in_external(0, 0)); // cos input route to first integrator

  TEST_ASSERT(cluster.route(0, 25, 1.0f, 2)); // integrate cos on first integrator to get sin
  TEST_ASSERT(cluster.route_out_external(2, 2, 1.0f));

  // Write configuration and calibrate
  TEST_ASSERT(lucidac.write_to_hardware());
  TEST_ASSERT(lucidac.calibrate_routes(&DAQ));
  /*
          ManualControl::init();
          ManualControl::to_ic();
          delayMicroseconds(1000);
          ManualControl::to_op();
  */

  // Start calulation
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 4'000'000, mode::OnOverload::IGNORE,
                                  mode::OnExtHalt::IGNORE));
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
  RUN_TEST(test);
  UNITY_END();
}

void loop() {}
