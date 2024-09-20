// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#include "mode/mode.h"
#include "protocol/jsonl_logging.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;

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

void test_overload() {
  for (auto &cluster : carrier_.clusters)
    for (auto mblock : {cluster.m0block, cluster.m1block}) {
      if (!mblock)
        continue;
      // Connect all zero to M-Block
      for (auto slot_idx : MBlock::SLOT_INPUT_IDX_RANGE())
        TEST_ASSERT(cluster.add_constant(blocks::UBlock::Transmission_Mode::POS_REF,
                                         mblock->slot_to_global_io_index(slot_idx), 0.0f,
                                         mblock->slot_to_global_io_index(slot_idx)));
      TEST_ASSERT(cluster.write_to_hardware());
      TEST_ASSERT(cluster.calibrate_offsets());

      // Reset overload
      mblock->hardware->reset_overload_flags();
      // Test overload
      printf("Testing M%d-block in cluster %d...\n", static_cast<uint8_t>(mblock->slot), cluster.cluster_idx);
      switch (static_cast<MBlock::TYPES>(mblock->get_entity_type())) {
      case MBlock::TYPES::UNKNOWN:
        TEST_FAIL_MESSAGE("Unknown M-Block.");
        break;
      case MBlock::TYPES::M_MUL4_BLOCK:
        for (auto slot_idx : MBlock::SLOT_INPUT_IDX_RANGE()) {
          cluster.cblock->set_factors({0});
          cluster.iblock->set_upscaling({0});
          TEST_ASSERT(cluster.cblock->set_factor(mblock->slot_to_global_io_index(slot_idx), 0.2f));
          TEST_ASSERT(cluster.iblock->set_upscaling(mblock->slot_to_global_io_index(slot_idx), true));
          TEST_ASSERT(cluster.write_to_hardware());

          mblock->hardware->reset_overload_flags();

          auto reading = mblock->hardware->read_overload_flags();
          TEST_ASSERT_EQUAL(1 << slot_idx, reading.to_ulong());
          delay(10);
        }
        break;
      case MBlock::TYPES::M_INT8_BLOCK:
        for (auto slot_idx : MBlock::SLOT_INPUT_IDX_RANGE()) {
          cluster.cblock->set_factors({0});
          TEST_ASSERT(cluster.cblock->set_factor(mblock->slot_to_global_io_index(slot_idx), 1.0f));
          TEST_ASSERT(cluster.cblock->write_to_hardware());

          mode::ManualControl::to_ic();
          delay(1);
          mblock->hardware->reset_overload_flags();
          mode::ManualControl::to_op();
          delay(1);
          mode::ManualControl::to_halt();

          auto reading = mblock->hardware->read_overload_flags();
          TEST_ASSERT_EQUAL(1 << slot_idx, reading.to_ulong());
          delay(10);
        }
        break;
      }
      delay(10);
    }
}

void setup() {
  msg::activate_serial_log();
  // Initialize things
  bus::init();
  // Ensure state changes as little as possible
  mode::ManualControl::init();
  mode::ManualControl::to_ic();

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_overload);
  UNITY_END();
}

void loop() { delay(500); }
