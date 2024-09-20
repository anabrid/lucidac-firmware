// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"
#include "protocol/jsonl_logging.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

const float target_precision = 0.1f; // Replace with something from test_common.h

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
  TEST_ASSERT(carrier_.write_to_hardware());
}

void test_calibration() {
  for (auto &cluster : carrier_.clusters)
    for (auto mblock : {cluster.m0block, cluster.m1block}) {
      if (!mblock or !mblock->is_entity_type(MBlock::TYPES::M_MUL4_BLOCK))
        continue;
      printf("Calibrating M%d-block in cluster %d...\n", static_cast<uint8_t>(mblock->slot),
             cluster.cluster_idx);
      TEST_ASSERT(carrier_.calibrate_mblock(cluster, *mblock, &DAQ));
      for (auto &calibration : static_cast<MMulBlock *>(mblock)->get_calibration())
        printf("Calibration: offset_x=%f, offset_y=%f, offset_z=%f\n", calibration.offset_x,
               calibration.offset_y, calibration.offset_z);
    }
}

void test_function(float x, float y) {
  // Calibration should not leave any connections dangling around
  // That's why the first time we need to get ourselves some reference signals
  static bool first_time = true;
  if (first_time) {
    first_time = false;
    for (auto &cluster : carrier_.clusters) {
      TEST_ASSERT(!cluster.ublock->is_anything_connected());
      TEST_ASSERT(!cluster.iblock->is_anything_connected());
    }
    // Route a constant x and y to each multiplier in the system
    for (auto &cluster : carrier_.clusters)
      for (auto mblock : {cluster.m0block, cluster.m1block}) {
        if (!mblock or !mblock->is_entity_type(MBlock::TYPES::M_MUL4_BLOCK))
          continue;
        for (auto slot_idx : MBlock::SLOT_INPUT_IDX_RANGE())
          TEST_ASSERT(cluster.add_constant(blocks::UBlock::Transmission_Mode::POS_REF,
                                           mblock->slot_to_global_io_index(slot_idx), 1.0f,
                                           mblock->slot_to_global_io_index(slot_idx)));
      }
    // Write to hardware and calibrate
    TEST_ASSERT(carrier_.write_to_hardware());
    TEST_ASSERT(carrier_.calibrate_routes(&DAQ));
  }

  // Set x and y by changing coefficients
  for (auto &cluster : carrier_.clusters)
    for (auto mblock : {cluster.m0block, cluster.m1block}) {
      if (!mblock or !mblock->is_entity_type(MBlock::TYPES::M_MUL4_BLOCK))
        continue;
      for (auto slot_idx : MBlock::SLOT_INPUT_IDX_RANGE())
        TEST_ASSERT(
            cluster.cblock->set_factor(mblock->slot_to_global_io_index(slot_idx), slot_idx % 2 ? x : y));
    }
  // Write to hardware and calibrate (but calibrating offsets is enough)
  TEST_ASSERT(carrier_.write_to_hardware());
  TEST_ASSERT(carrier_.calibrate_offset());

  // Route outputs to ADC and check for the expected value
  bool error = false;
  for (auto &cluster : carrier_.clusters)
    for (auto mblock : {cluster.m0block, cluster.m1block}) {
      if (!mblock or !mblock->is_entity_type(MBlock::TYPES::M_MUL4_BLOCK))
        continue;
      for (auto mul_idx : MMulBlock::MULTIPLIERS_OUTPUT_RANGE())
        TEST_ASSERT(carrier_.set_adc_channel(mul_idx, mblock->slot_to_global_io_index(mul_idx)));
      TEST_ASSERT(carrier_.write_to_hardware());

      auto data = DAQ.sample();
      for (auto mul_idx : MMulBlock::MULTIPLIERS_OUTPUT_RANGE()) {
        if (fabs(data[mul_idx] - x * y) > target_precision) {
          error = true;
          printf(
              "ERROR cluster=%d mblock-slot=%d mul_idx=%d output=%f outside acceptable range! Expected=%f\n",
              cluster.cluster_idx, static_cast<uint8_t>(mblock->slot), mul_idx, data[mul_idx], x * y);
        }
      }
    }
  TEST_ASSERT_FALSE(error);
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);
  msg::activate_serial_log();

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_calibration);
  RUN_PARAM_TEST(test_function, 1.0f, 1.0f);
  RUN_PARAM_TEST(test_function, 0.75f, 0.75f);
  RUN_PARAM_TEST(test_function, 0.5f, 0.5f);
  RUN_PARAM_TEST(test_function, 0.1f, 0.1f);
  RUN_PARAM_TEST(test_function, 1.0f, -1.0f);
  RUN_PARAM_TEST(test_function, 0.75f, -0.75f);
  RUN_PARAM_TEST(test_function, 0.5f, -0.5f);
  RUN_PARAM_TEST(test_function, 0.1f, -0.1f);
  RUN_PARAM_TEST(test_function, -1.0f, -1.0f);
  RUN_PARAM_TEST(test_function, -0.75f, -0.75f);
  RUN_PARAM_TEST(test_function, -0.5f, -0.5f);
  RUN_PARAM_TEST(test_function, -0.1f, -0.1f);
  UNITY_END();
}

void loop() { delay(500); }
