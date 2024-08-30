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

// This test case is doing some simple ramp test cases, so it's integrating a constant. Currently there isn't
// much variety or automation, but this is planned, once FlexIO allows for IC setting in slow integration mode
// Currently this only tests the M0 slot

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

void setup_and_measure(uint8_t channel) {
  // Setup path
  carrier_.ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());
  cluster.reset(false);

  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, channel, 0.1f, channel));

  auto int_block = static_cast<MIntBlock *>(cluster.m0block);
  TEST_ASSERT(int_block->set_time_factor(channel, 10000));
  TEST_ASSERT(int_block->set_ic_value(channel, 0.0f));

  TEST_ASSERT(carrier_.write_to_hardware());

  TEST_ASSERT(cluster.calibrate(&DAQ));

  carrier_.ctrl_block->set_adc_bus(CTRLBlock::ADCBus::ADC);
  TEST_ASSERT(carrier_.set_adc_channel(channel, channel));
  TEST_ASSERT(carrier_.write_to_hardware());

  // Measure end value
  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 1'000'000, mode::OnOverload::IGNORE,
                                  mode::OnExtHalt::IGNORE));
  FlexIOControl::force_start();
  while (!FlexIOControl::is_done()) {
  }

  float reading = DAQ.sample_avg(4, 10)[channel];

  TEST_ASSERT_FLOAT_WITHIN(target_precision, -1.0f, reading);
  std::cout << "channel " << +channel << " passed!" << std::endl;
}

void check_all_integrators() {
  for (uint8_t i : MIntBlock::INTEGRATORS_OUTPUT_RANGE()) {
    std::cout << "Testing channel " << +i << std::endl;
    setup_and_measure(i);
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
  RUN_TEST(check_all_integrators);
  UNITY_END();
}

void loop() {}
