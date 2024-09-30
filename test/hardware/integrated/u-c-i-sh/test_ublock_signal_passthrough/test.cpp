// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#define private public
#define protected public
#include "lucidac/lucidac.h"

#include "test_common.h"
#include "test_parametrized.h"

#include "daq/daq.h"

using namespace platform;
using namespace blocks;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

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
  carrier_.reset(entities::ResetAction::EVERYTHING);
  TEST_ASSERT(carrier_.write_to_hardware());

  // For this test case, we always use reference signals on all U-block inputs
  for (auto &cluster : carrier_.clusters) {
    cluster.ublock->change_all_transmission_modes(UBlock::Transmission_Mode::POS_REF);
    // C block reset to 1
    for (auto i_in_idx : IBlock::INPUT_IDX_RANGE())
      TEST_ASSERT(cluster.iblock->connect(i_in_idx, i_in_idx / 2));
  }
}

void test_ublock_inoutconnect(uint8_t input, uint8_t output) {
  std::cout << "Checking U-Block connection " << static_cast<unsigned int>(input) << "->"
            << static_cast<unsigned int>(output) << std::endl;
  for (auto &cluster : carrier_.clusters) {
    cluster.ublock->reset_connections();
    // Use low-level _connect(), since connect() may reset transmission mode
    cluster.ublock->_connect(input, output);
    TEST_ASSERT(cluster.write_to_hardware());

    auto data = measure_sh_gain(carrier_.ctrl_block, cluster, &DAQ);
    std::cout << "DATA = " << data << std::endl;
    for (auto idx = 0u; idx < data.size(); idx++) {
      if (idx == output / 2)
        // Check expected output against 0.95 due to decreasing I-block gain on summation
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.15, 0.95, data[idx],
                                         "A connected output is not as expected, there may be connectivity "
                                         "issues on the U-block. Check DATA in the log.");
      else
        // Check whether all others are zero (may detect whether short-circuits are present)
        TEST_ASSERT_FLOAT_WITHIN_MESSAGE(0.2, 0, data[idx],
                                         "An unconnected output is not zero, there may be a short on the "
                                         "U-Block output side. Check DATA in the log.");
    }
  }
}

void test_all_combinations() {
  for (auto u_input : UBlock::INPUT_IDX_RANGE())
    for (auto u_output : UBlock::OUTPUT_IDX_RANGE())
      test_ublock_inoutconnect(u_input, u_output);
}

void setup() {
  bus::init();
  msg::activate_serial_log();
  DAQ.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_all_combinations);
  // RUN_PARAM_TEST(test_ublock_inoutconnect, 0, 0);
  // RUN_PARAM_TEST(test_ublock_inoutconnect, 0, 1);
  UNITY_END();
}

void loop() { delay(500); }
