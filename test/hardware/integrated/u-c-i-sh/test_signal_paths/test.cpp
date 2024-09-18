// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#include "test_common.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

#include "utils/running_avg.h"

using namespace platform;
using namespace blocks;

bool print_details = false;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

typedef std::vector<uint8_t> I;

utils::RunningAverage<float> avg_devations;

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
  // Reset
  carrier_.reset(false);
  TEST_ASSERT(carrier_.write_to_hardware());
}

// Workaround for RUN_TEST
std::array<float, 32> factors;
std::array<I, 16> connections;
bool full_calibration;

void test_summation() {
  if (print_details)
    std::cout << "factor = " << factors[0] << std::endl; // All factors are the same
  if (print_details)
    std::cout << "connections = " << connections << std::endl;

  // Calculate expected sum
  std::array<float, 16> expected{};
  std::array<float, 16> deviations{};
  for (auto i_out_idx = 0u; i_out_idx < connections.size(); i_out_idx++) {
    for (uint8_t i_in_idx : connections[i_out_idx]) {
      expected[i_out_idx] += factors[i_in_idx];
    }
  }
  if (print_details)
    std::cout << "expected = " << expected << std::endl;

  unsigned int relevant_i_out = 0;

  // Configure on hardware
  for (auto &cluster : carrier_.clusters) {
    carrier_.ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());
    TEST_ASSERT(carrier_.ctrl_block->write_to_hardware());

    // Connect reference through U-C-I to SH
    cluster.ublock->reset_connections();
    for (auto i_out_idx = 0u; i_out_idx < connections.size(); i_out_idx++) {
      for (uint8_t i_in_idx : connections[i_out_idx]) {
        relevant_i_out = i_out_idx; // We only have one output we can use at a time in automatic mode
        TEST_ASSERT(
            cluster.add_constant(UBlock::Transmission_Mode::POS_REF, i_in_idx, factors[i_in_idx], i_out_idx));
      }
    }

    TEST_ASSERT(cluster.write_to_hardware());

    // As soon as we use a new route, the cluster should be calibrated completly
    if (full_calibration)
      TEST_ASSERT(cluster.calibrate_routes(&DAQ));
    else
      TEST_ASSERT(cluster.calibrate_offsets());

    // Measure by using SH gain outputs
    auto data = measure_sh_gain(cluster, &DAQ);
    if (print_details)
      std::cout << "data = " << data << std::endl;

    for (auto idx = 0u; idx < data.size(); idx++) {
      TEST_ASSERT_FLOAT_WITHIN(full_calibration ? 0.01f : 0.05f, expected[idx], data[idx]);
      deviations[idx] = data[idx] - expected[idx];
    }

    if (print_details)
      std::cout << "fullscale deviation in per mille = " << deviations[relevant_i_out] * 1000.0f << std::endl;

    avg_devations.add(deviations[relevant_i_out] * 1000.0f);
  }
}

void test_n_summations() {
  for (auto N : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u}) {
    std::fill(factors.begin(), factors.end(), 1.0f / static_cast<float>(N));
    for (auto i_out_idx : IBlock::OUTPUT_IDX_RANGE()) {
      for (auto i_in_shift = 0u; i_in_shift <= 32u - N; i_in_shift += N) {
        std::for_each(connections.begin(), connections.end(), [](I &i) { i.clear(); });
        for (auto i_in_idx = 0u; i_in_idx < N; i_in_idx++)
          connections[i_out_idx].emplace_back(i_in_idx + i_in_shift);
        // Run test on this configuration
        TEST_MESSAGE("--------------------------------------");
        std::cout << "Testing " << N << " connections" << std::endl;
        std::cout << "connections = " << connections << std::endl;
        carrier_.reset(false);
        full_calibration = true; // Since we allways change the signal path,

        RUN_TEST(test_summation);
      }
    }

    std::cout << "Average full scale deviation in per mille in this cycle: " << avg_devations.get_average()
              << std::endl;
  }
}

void setup() {
  bus::init();
  io::init();
  // msg::activate_serial_log();
  DAQ.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_n_summations);
  UNITY_END();
}

void loop() { delay(500); }
