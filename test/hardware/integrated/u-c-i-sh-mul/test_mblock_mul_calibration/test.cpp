// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "test_fmtlib.h"

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

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
}

void test_calibration() {
  for (auto &cluster : carrier_.clusters)
    for (auto mblock : {cluster.m0block, cluster.m1block}) {
      if (!mblock or !mblock->is_entity_type(MBlock::TYPES::M_MUL4_BLOCK))
        continue;
      TEST_MESSAGE_FORMAT("Calibrating M{}-block in cluster {}...", static_cast<uint8_t>(mblock->slot),
                          cluster.cluster_idx);
      TEST_ASSERT(carrier_.calibrate_mblock(cluster, *mblock, &DAQ));
      for (auto &calibration : static_cast<MMulBlock *>(mblock)->get_calibration())
        TEST_MESSAGE_FORMAT("Calibration: offset_x={}, offset_y={}, offset_z={}", calibration.offset_x,
                            calibration.offset_y, calibration.offset_z);
    }
}

void test_function() {
  // Calibration should not leave any connections dangling around
  for (auto &cluster : carrier_.clusters) {
    TEST_ASSERT(!cluster.ublock->is_anything_connected());
    TEST_ASSERT(!cluster.iblock->is_anything_connected());
  }
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_calibration);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() { delay(500); }
