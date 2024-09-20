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
// case basically tells you all you need to do. The CTRL-Block button should be pressed down for something like
// a second to be registered.

using namespace platform;
using namespace blocks;
using namespace mode;

int get_mint_potentiometer_number(uint8_t channel, bool slow) { return channel * 2 + 1 + slow; }

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

  TEST_ASSERT(carrier_.write_to_hardware());
}

Cluster *cluster; // Workaround for unitys inability to have parametrised tests.
MIntBlock *int_block;

void setup_and_measure(uint8_t channel, bool use_slow_integration) {
  carrier_.reset(entities::ResetAction::CIRCUIT_RESET);

  TEST_ASSERT(cluster->add_constant(UBlock::Transmission_Mode::POS_REF,
                                    int_block->slot_to_global_io_index(channel), 1.0f,
                                    int_block->slot_to_global_io_index(channel)));

  TEST_ASSERT(int_block->set_time_factors(use_slow_integration ? 100 : 10000));
  TEST_ASSERT(int_block->set_ic_value(channel, 0.0f));

  TEST_ASSERT(carrier_.set_adc_channel(channel, int_block->slot_to_global_io_index(channel)));

  TEST_ASSERT(carrier_.write_to_hardware());
  TEST_ASSERT(carrier_.calibrate_routes(&DAQ));

  while (true) {
    TEST_ASSERT(cluster->calibrate_offsets());

    TEST_ASSERT(FlexIOControl::init(use_slow_integration ? mode::DEFAULT_IC_TIME * 100 : mode::DEFAULT_IC_TIME,
                                    use_slow_integration ? 10'000'000 : 100'000, mode::OnOverload::IGNORE,
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

    std::cout << "Reading channel " << +channel << " in " << (use_slow_integration ? "slow" : "fast")
              << " mode:  ";
    if (fabs(reading + 1.0f) < target_precision)
      std::cout << "Channel is within machine accurary. Press CTRL-block button to switch to next channel. ";
    else
      std::cout << "Turn potentiometer RV" << get_mint_potentiometer_number(channel, use_slow_integration)
                << " " << direction << ". ";
    std::cout << " Read value: " << reading << std::endl;

    delay(1000);
    FlexIOControl::reset();

    if (io::get_button()) {
      std::cout << "Switching channel" << std::endl;
      delay(2000);
      return;
    }
  }
}

void calibrate_all() {
  for (Cluster &cluster_it : carrier_.clusters) {
    for (MBlock *m_block : {cluster_it.m0block, cluster_it.m1block}) {
      if (!m_block || !m_block->is_entity_type(MBlock::TYPES::M_INT8_BLOCK))
        continue;

      cluster = &cluster_it;
      int_block = static_cast<MIntBlock *>(m_block);

      std::cout << "Testing cluster " << +cluster_it.get_cluster_idx() << " slot " << int(m_block->slot)
                << ": " << std::endl;

      for (int ch = 0; ch < MIntBlock::NUM_INTEGRATORS; ch++) {
        setup_and_measure(ch, false);
        setup_and_measure(ch, true);
      }
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
  RUN_TEST(calibrate_all);
  UNITY_END();
}

void loop() {}
