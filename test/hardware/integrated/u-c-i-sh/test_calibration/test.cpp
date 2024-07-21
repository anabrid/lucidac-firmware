// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "test_fmtlib.h"
#include <Arduino.h>
#include <unity.h>

#include "daq/daq.h"
#include "io/io.h"

#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;
blocks::CTRLBlockHAL_V_1_0_1 ctrlblock_hal;
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
}

void test_calibration() {
  // Connect ADC_BUS on CTRL-Block
  ctrlblock_hal.write_adc_bus_muxers(blocks::CTRLBlock::ADCBus::CL0_GAIN);
  // io::block_until_button_press();

  for (auto &cluster : carrier_.clusters) {
    TEST_ASSERT(cluster.route(2, 0, 0.3f, 1));
    TEST_ASSERT(cluster.route(1, 1, -0.2f, 2));
    TEST_ASSERT(cluster.add_constant(UBlock::POS_BIG_REF, 3, 0.114f, 1));

    TEST_ASSERT(cluster.write_to_hardware());

    TEST_ASSERT(cluster.calibrate(&DAQ));

    // Check whether all gain corrections are in a reasonable range
    TEST_MESSAGE_FORMAT("Gain corrections are {}.", cluster.cblock->get_gain_corrections());
    for (auto gain_correction: cluster.cblock->get_gain_corrections())
      TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, gain_correction);

    // Check whether all connections have been restored
    TEST_ASSERT(cluster.ublock->is_connected(2, 0));
    TEST_ASSERT(cluster.iblock->is_connected(0, 1));
    TEST_ASSERT(cluster.ublock->is_connected(1, 1));
    TEST_ASSERT(cluster.iblock->is_connected(1, 2));
    TEST_ASSERT(cluster.ublock->is_connected(15, 3));
    TEST_ASSERT(cluster.iblock->is_connected(3, 1));
  }
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_calibration);
  UNITY_END();
}

void loop() { delay(500); }
