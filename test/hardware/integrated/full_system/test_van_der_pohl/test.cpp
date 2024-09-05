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

auto config =
    R"({
  "/0": {
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
        ],
        [
          2
        ],
        [],
        [],
        [],
        [],
        [],
        [],
        [
          3
        ],
        [
          4
        ],
        [],
        [],
        [
          5,
          16
        ],
        [
          6
        ],
        [],
        []
      ],
      "upscaling": [
        true,
        false,
        true,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false,
        false
      ]
    },
    "/M0": {
      "elements": [
        {
          "k": 10000,
          "ic": 0
        },
        {
          "k": 10000,
          "ic": 0.1
        },
        {
          "k": 10000,
          "ic": 0
        },
        {
          "k": 10000,
          "ic": 0
        },
        {
          "k": 10000,
          "ic": 0
        },
        {
          "k": 10000,
          "ic": 0
        },
        {
          "k": 10000,
          "ic": 0
        },
        {
          "k": 10000,
          "ic": 0
        }
      ]
    },
    "/M1": {}
  },
  "adc_channels": [
    0,
    1,
    10,
    8,
    null,
    null,
    null,
    null
  ]
})";

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

void setup_and_measure() {
  StaticJsonDocument<5000> doc;
  TEST_ASSERT(DeserializationError::Ok == deserializeJson(doc, config));

  JsonObject cfg = doc.as<JsonObject>();
  TEST_ASSERT_FALSE(cfg.isNull());

  auto int_block = static_cast<MIntBlock *>(cluster.m0block);
  auto mul_block = static_cast<MMulBlock *>(cluster.m1block);

  auto res = lucidac.calibrate_mblock(cluster, *mul_block, &DAQ);

  if(!res) {
    LOGMEV("Calibrate mblock return code: %d -- message: %s", res.code, res.msg.c_str());
  }

  TEST_ASSERT(res);

  TEST_ASSERT(lucidac.config_from_json(cfg));
  TEST_ASSERT(lucidac.write_to_hardware());

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
