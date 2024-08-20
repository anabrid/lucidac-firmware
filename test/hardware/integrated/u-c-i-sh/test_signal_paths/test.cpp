// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "test_common.h"
#include "test_fmtlib.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;
blocks::CTRLBlockHAL_V_1_0_2 ctrlblock_hal;
daq::OneshotDAQ DAQ;

typedef std::vector<uint8_t> I;

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
}

void test_summation(const std::array<float, 32> &factors, const std::array<I, 16> &connections,
                    bool full_calibration) {
  TEST_MESSAGE_FORMAT("factor={}", factors[0]); // All factors are the same
  TEST_MESSAGE_FORMAT("connections={}", connections);
  // Calculate expected sum
  std::array<float, 16> expected{};
  std::array<float, 16> deviations{};
  for (auto i_out_idx = 0u; i_out_idx < connections.size(); i_out_idx++) {
    for (uint8_t i_in_idx : connections[i_out_idx]) {
      expected[i_out_idx] += factors[i_in_idx];
    }
  }
  TEST_MESSAGE_FORMAT("expected={}", expected);

  unsigned int relevant_i_out = 0;

  // Configure on hardware
  for (auto &cluster : carrier_.clusters) {
    carrier_.ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());

    // Connect reference through U-C-I to SH
    cluster.ublock->reset_connections();
    for (auto i_out_idx = 0u; i_out_idx < connections.size(); i_out_idx++) {
      for (uint8_t i_in_idx : connections[i_out_idx]) {
        relevant_i_out = i_out_idx; // We only have one output we can use at a time
        TEST_ASSERT(
            cluster.add_constant(UBlock::Transmission_Mode::POS_REF, i_in_idx, factors[i_in_idx], i_out_idx));
      }
    }
    TEST_ASSERT(cluster.write_to_hardware());
    // We only calibrate offsets, since we want this test case to be simple
    if (full_calibration) {
      cluster.cblock->reset_gain_corrections();
      TEST_ASSERT(cluster.calibrate(&DAQ));
    } else
      TEST_ASSERT(cluster.calibrate_offsets());

    // Measure by using SH gain outputs
    auto data = measure_sh_gain(cluster, &DAQ);
    TEST_MESSAGE_FORMAT("data={}", data);

    for (auto idx = 0u; idx < data.size(); idx++) {
      TEST_ASSERT_FLOAT_WITHIN(full_calibration ? 0.1f : 0.05f, expected[idx], data[idx]);
      deviations[idx] = data[idx] - expected[idx];
    }

    TEST_MESSAGE_FORMAT("absolute deviation ={}",
                        deviations[relevant_i_out]); // this is absolute, but since we sum to one, relative and
                                                     // absolute deviation are the same
  }
}

void test_n_summations() {
  for (auto N : {5u}) {
    std::array<float, 32> factors{};
    std::fill(factors.begin(), factors.end(), 1.0f / static_cast<float>(N));
    for (auto i_out_idx : {0, 1, 2, 3, 4}) {
      for (auto i_in_shift = 0u; i_in_shift < 32u - N; i_in_shift += N) {
        std::array<I, 16> connections;
        for (auto i_in_idx = 0u; i_in_idx < N; i_in_idx++)
          connections[i_out_idx].emplace_back(i_in_idx + i_in_shift);
        // Run test on this configuration
        TEST_MESSAGE("--------------------------------------");
        TEST_MESSAGE_FORMAT("Testing = {} connections", N);
        carrier_.reset(true);
        test_summation(factors, connections, i_out_idx == 0);
      }
    }
  }
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_PARAM_TEST(test_summation,
                 {0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8,
                  0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8},
                 {I{1}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}, I{}}, false);
  RUN_TEST(test_n_summations);
  UNITY_END();
}

void loop() { delay(500); }
