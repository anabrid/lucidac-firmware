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

// This test tries to compute the trigonometric pythagorian theorem sin(x)^2 + cos(x)^2 = 1.
// Running this calculation tests every aspect of the LUCIDAC we currently have, as it requieres the signal
// generator, the integrator, the multiplier and the implicit summation in the I block.

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
  auto mul_block = static_cast<MMulBlock *>(cluster.m1block);

  auto &signal_generator = lucidac.front_panel->signal_generator;
  signal_generator.set_frequency(10000.0f /
                                 (2.0f * 3.14f)); // create cosine with omega = 1 on integrators time scale
  signal_generator.set_phase(3.14f / 2.0f);
  signal_generator.set_amplitude(1.0f);
  signal_generator.set_offset(0.02f); // currently we can only guess and adjust the input offset by hand
  signal_generator.awake();

  TEST_ASSERT(int_block->set_time_factors(10000));

  // Setup actual circuit for calulation
  TEST_ASSERT(lucidac.set_acl_select(0, LUCIDAC::ACL::EXTERNAL_));
  TEST_ASSERT(cluster.route_in_external(0, 8)); // cos input route to first m-mul slot to use id function

  // integrate cos on first integrator to get sin
  TEST_ASSERT(cluster.route(12, 25, 1.0f, 0));

  TEST_ASSERT(cluster.route_out_external(0, 2, 1.0f)); // sin measurement output

  // Multiply cos together
  TEST_ASSERT(cluster.route(12, 5, 1.0f, 10)); // x1 mul
  TEST_ASSERT(cluster.route(12, 6, 1.0f, 11)); // y1 mul

  TEST_ASSERT(cluster.route_out_external(9, 3, 1.0f)); // cos^2 measurement output

  // Multiply sin together
  TEST_ASSERT(cluster.route(0, 7, 1.0f, 12)); // x2 mul
  TEST_ASSERT(cluster.route(0, 8, 1.0f, 13)); // y2 mul

  TEST_ASSERT(cluster.route_out_external(10, 4, 1.0f)); // sin^2 measurement output

  // Sum together sin^2 and cos^2
  TEST_ASSERT(cluster.route(9, 9, 1.0f, 9));   // route sin^2 to y0 for id function
  TEST_ASSERT(cluster.route(10, 10, 1.0f, 9)); // route cos^2 to y0 for id function

  TEST_ASSERT(cluster.route_out_external(13, 5, -1.0f)); // sin^2 + cos^2 measurement output

  // Write configuration and calibrate
  TEST_ASSERT(lucidac.write_to_hardware());
  TEST_ASSERT(lucidac.calibrate_routes(&DAQ));

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
