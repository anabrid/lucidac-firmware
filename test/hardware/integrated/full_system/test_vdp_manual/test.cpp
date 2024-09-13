// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "test_common.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"
#include "mode/mode.h"

#include "lucidac/lucidac.h"

#include "utils/running_avg.h"

bool extra_logs = true;

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

  lucidac.reset(false);
  TEST_ASSERT(lucidac.write_to_hardware());
}

/*
"/U": {
      "outputs": [
        10,
        1,
        0,
        1,
        1,
        8,
        0,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        14,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        null,
        0,
        1,
        null
      ],
      "constant": true
    },
    "/C": {
      "elements": [
        -0.4,
        -0.5,
        0.2,
        1,
        1,
        -1,
        1,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0.25,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        0,
        1,
        1,
        0
      ]
    },
    "/I": {
      "outputs": [
        [
          0,
          1
        ],0
        [
          2
        ],1
        [], 2
        [],3
        [],4
        [],5
        [],6
        [],7
        [
          3
        ],8
        [
          4
        ],9
        [],10
        [],11
        [
          5,
          16
        ],12
        [
          6
        ],13
        [],
        []
      ]
          */

void setup_and_measure() {
  cluster.ublock->connect(11, 0);
  /*
  cluster.ublock->connect(1, 1);
  cluster.ublock->connect(0, 2);
  cluster.ublock->connect(1, 3);
  cluster.ublock->connect(1, 4);
  cluster.ublock->connect(8, 5);
  cluster.ublock->connect(0, 6);
  cluster.ublock->connect(14, 16);
  cluster.ublock->connect(0, 29);
  cluster.ublock->connect(1, 30);
  */

  cluster.cblock->set_factor(-0.4f, 0);
  /*
  cluster.cblock->set_factor(-0.5f, 1);
  cluster.cblock->set_factor(0.2f, 2);
  cluster.cblock->set_factor(1.0f, 3);
  cluster.cblock->set_factor(1.0f, 4);
  cluster.cblock->set_factor(-1.0f, 5);
  cluster.cblock->set_factor(1.0f, 6);
  cluster.cblock->set_factor(0.25f, 16);
  cluster.cblock->set_factor(1.0f, 29);
  cluster.cblock->set_factor(1.0f, 30);
  */

  cluster.iblock->connect(0, 0);
  /*
  cluster.iblock->connect(1, 0);
  cluster.iblock->connect(2, 1);
  cluster.iblock->connect(3, 8);
  cluster.iblock->connect(4, 9);
  cluster.iblock->connect(5, 12);
  cluster.iblock->connect(16,12);
  cluster.iblock->connect(6, 13);
*/
  TEST_ASSERT(lucidac.write_to_hardware());

  Serial.println("Calibrating routes: ");
  TEST_ASSERT(lucidac.calibrate_routes(&DAQ));

  TEST_ASSERT(FlexIOControl::init(mode::DEFAULT_IC_TIME, 8'000'000, mode::OnOverload::IGNORE,
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
  RUN_TEST(setup_and_measure);
  UNITY_END();
}

void loop() {}
