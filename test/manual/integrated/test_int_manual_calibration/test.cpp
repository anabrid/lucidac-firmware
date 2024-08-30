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
const float target_precision = 0.001f; // Replace with something from test_common.h

// This test case is dedicated to calibrating the MInt blocks with real potentiometers. Starting up the test
// case basically tells you all you need to know. The Int block is expected to be placed in the 0 slot.
// The CTRL-Block button should be pressed down for something like a second to be registered.

// Currently this test case doesn't have much error handling, so broken hardware can result in a test failure
// and manual (hardware) debugging is needed. In this case you can set extra_logs to true.
// Also this test case currently only is able to calibrate the fast integration path, as the slow IC signals
// can't be set with FlexIO.

using namespace platform;
using namespace blocks;
using namespace mode;

int get_mint_potentiometer_number(uint8_t channel, bool slow) { return channel * 2 + 1 + slow; }

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

void start_and_measure(uint8_t channel) {
  while (true) {
    TEST_ASSERT(cluster.calibrate_offsets());

    TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 1'000'000, mode::OnOverload::IGNORE,
                                    mode::OnExtHalt::IGNORE));
    FlexIOControl::force_start();
    while (!FlexIOControl::is_done()) {
    }

    float reading = DAQ.sample_avg(4, 10)[channel];
    std::string direction;
    if (reading < -1.0f)
      direction = "clockwise";
    else
      direction = "counterclockwise";

    std::cout << "Reading channel " << +channel << ":  ";
    if (fabs(reading + 1.0f) < target_precision)
      std::cout << "Channel is within machine accurary. Press CTRL-block button to switch to next channel. ";
    else
      std::cout << "Turn potentiometer RV" << get_mint_potentiometer_number(channel, false) << " " << direction
                << ". ";
    std::cout << " Read value: " << reading << std::endl;

    delayMicroseconds(1000000);
    FlexIOControl::reset();

    if (io::get_button()) {
      std::cout << "Switching channel" << std::endl;
      delay(2000);
      return;
    }
  }
}

void setup_integration(uint8_t channel) {
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

  start_and_measure(channel);
}

void calibrate_all_integrators() {
  for (uint8_t i : MIntBlock::INTEGRATORS_OUTPUT_RANGE())
    setup_integration(i);
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);
  if (extra_logs)
    msg::activate_serial_log();

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(calibrate_all_integrators);
  UNITY_END();
}

void loop() {}
