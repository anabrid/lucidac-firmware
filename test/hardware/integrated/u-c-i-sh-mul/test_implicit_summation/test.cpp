// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public

#include "test_common.h"
#include "test_fmtlib.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"

#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

LUCIDAC carrier_;
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
    TEST_ASSERT_NOT_NULL(cluster.m1block);
  }
  TEST_ASSERT_NOT_NULL(carrier_.ctrl_block);

  carrier_.reset(false);
}

void test_summation() {
  auto &cluster = carrier_.clusters[0];

  // carrier_.ctrl_block->set_adc_bus_to_cluster_gain(cluster.get_cluster_idx());

  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 0, 1.0f, 8));
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 1, 1.0f, 9));

  TEST_ASSERT(cluster.write_to_hardware());

  // cluster.cblock->reset_gain_corrections();
  // TEST_ASSERT(cluster.calibrate(&DAQ));

  carrier_.ctrl_block->set_adc_bus(CTRLBlock::ADCBus::ADC);
  TEST_ASSERT(carrier_.write_to_hardware());

  for (int i = 0; i < 8; i++)
    TEST_ASSERT(carrier_.set_adc_channel(i, 8 + i));

  TEST_ASSERT(carrier_.write_to_hardware());

  delay(50);

  auto sample = DAQ.sample();
  TEST_MESSAGE_FORMAT("Read In = {}", sample);
}

void setup() {
  bus::init();
  io::init();
  DAQ.init(0);

  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_summation);
  UNITY_END();
}

void loop() { delay(500); }
