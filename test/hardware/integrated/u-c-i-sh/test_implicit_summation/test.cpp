// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include <iomanip>
#include <iostream>

#define private public
#define protected public

#include "test_common.h"
#include "test_parametrized.h"

#include "daq/daq.h"
#include "io/io.h"

#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

LUCIDAC carrier_;
daq::OneshotDAQ DAQ;

auto &cluster = carrier_.clusters[0];

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

  carrier_.reset(false);
  TEST_ASSERT(carrier_.write_to_hardware());
}

void test_finished_summation(float x, float y) {
  cluster.reset(true);
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 0, x, 0));
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 1, y, 0));

  // Correct offset
  cluster.ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::GROUND);
  TEST_ASSERT(cluster.write_to_hardware());
  delay(10);
  cluster.shblock->set_track.trigger();
  delay(10);
  cluster.shblock->set_inject.trigger();

  cluster.shblock->set_gain.trigger();
  delay(5);
  // std::cout << "Offset correction: " << measure_sh_gain(cluster, &DAQ, 4, 10) << std::endl;

  cluster.ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::POS_REF);
  TEST_ASSERT(cluster.write_to_hardware());

  cluster.shblock->set_gain.trigger();

  TEST_ASSERT(carrier_.write_to_hardware());

  auto data = measure_sh_gain(cluster, &DAQ, 4, 10);
  std::cout << std::fixed << std::setprecision(4) << x << " + " << y << ": \t\t read in = " << data[0]
            << " \t\t expected result: " << x + y
            << " \t\t full scale deviation in per mille: " << (data[0] - (x + y)) * 1000.0f << std::endl;

  delay(10);
}

void calibrate_summation_factors() {}

void test_summation() {
  carrier_.ctrl_block->set_adc_bus_to_cluster_gain(0);

  // Calibrate Input 0
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 0, 1.0f, 0));
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 1, 0.0f, 0));

  TEST_ASSERT(cluster.write_to_hardware());
  // Correct offset
  cluster.ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::GROUND);
  TEST_ASSERT(cluster.write_to_hardware());
  delay(10);
  cluster.shblock->set_track.trigger();
  delay(10);
  cluster.shblock->set_inject.trigger();

  cluster.shblock->set_gain.trigger();
  delay(5);
  std::cout << "Offset correction: " << measure_sh_gain(cluster, &DAQ, 4, 10) << std::endl;

  // Correct gain
  cluster.ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::POS_REF);
  TEST_ASSERT(cluster.write_to_hardware());

  cluster.shblock->set_gain.trigger();

  TEST_ASSERT(carrier_.write_to_hardware());

  delay(10);

  auto data = measure_sh_gain(cluster, &DAQ, 4, 10);
  std::cout << "Read In = " << data << std::endl;

  // calulate and test correction
  TEST_ASSERT(cluster.cblock->set_gain_correction(0, 1.0f / data[0]));
  TEST_ASSERT(cluster.cblock->set_factor(0, 0.5f));
  TEST_ASSERT(carrier_.write_to_hardware());

  delay(10);

  data = measure_sh_gain(cluster, &DAQ, 4, 10);
  std::cout << "Corrected first Coefficient = " << data[0] << std::endl;

  // Delete first route
  // Calibrate Input 1

  cluster.reset(true);
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 0, 0.0f, 0));
  TEST_ASSERT(cluster.add_constant(UBlock::Transmission_Mode::POS_REF, 1, 1.0f, 0));

  TEST_ASSERT(cluster.write_to_hardware());
  // Correct offset
  cluster.ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::GROUND);
  TEST_ASSERT(cluster.write_to_hardware());
  delay(10);
  cluster.shblock->set_track.trigger();
  delay(10);
  cluster.shblock->set_inject.trigger();

  cluster.shblock->set_gain.trigger();
  delay(5);
  std::cout << "Offset correction: " << measure_sh_gain(cluster, &DAQ, 4, 10) << std::endl;

  // Correct gain
  cluster.ublock->change_b_side_transmission_mode(UBlock::Transmission_Mode::POS_REF);
  TEST_ASSERT(cluster.write_to_hardware());

  cluster.shblock->set_gain.trigger();

  TEST_ASSERT(carrier_.write_to_hardware());

  delay(10);

  data = measure_sh_gain(cluster, &DAQ, 4, 10);
  std::cout << "Read In = " << data << std::endl;

  // calulate and test correction
  TEST_ASSERT(cluster.cblock->set_gain_correction(1, 1.0f / data[0]));
  TEST_ASSERT(cluster.cblock->set_factor(1, 0.5f));
  TEST_ASSERT(carrier_.write_to_hardware());

  delay(10);

  data = measure_sh_gain(cluster, &DAQ, 4, 10);
  std::cout << "Corrected second Coefficient = " << data[0] << std::endl;

  // Restore summation route
  test_finished_summation(0.5f, 0.5f);
  test_finished_summation(1.0f, 0.0f);
  test_finished_summation(0.5f, 0.0f);
  test_finished_summation(0.3f, 0.0f);
  test_finished_summation(0.0f, 1.0f);
  test_finished_summation(0.75f, 0.25f);
  test_finished_summation(0.25f, 0.75f);
  test_finished_summation(0.33f, 0.67f);
  test_finished_summation(0.2f, 0.8f);
  test_finished_summation(0.0f, 0.0f);

  std::cout << std::endl;

  test_finished_summation(0.5f * 0.5f, 0.5f * 0.5f);
  test_finished_summation(0.5f * 1.0f, 0.5f * 0.0f);
  test_finished_summation(0.5f * 0.5f, 0.5f * 0.0f);
  test_finished_summation(0.5f * 0.3f, 0.5f * 0.0f);
  test_finished_summation(0.5f * 0.0f, 0.5f * 1.0f);
  test_finished_summation(0.5f * 0.75f, 0.5f * 0.25f);
  test_finished_summation(0.5f * 0.25f, 0.5f * 0.75f);
  test_finished_summation(0.5f * 0.33f, 0.5f * 0.67f);
  test_finished_summation(0.5f * 0.2f, 0.5f * 0.8f);
  test_finished_summation(0.0f, 0.0f);

  std::cout << std::endl;

  test_finished_summation(0.1f * 0.5f, 0.1f * 0.5f);
  test_finished_summation(0.1f * 1.0f, 0.1f * 0.0f);
  test_finished_summation(0.1f * 0.5f, 0.1f * 0.0f);
  test_finished_summation(0.1f * 0.3f, 0.1f * 0.0f);
  test_finished_summation(0.1f * 0.0f, 0.1f * 1.0f);
  test_finished_summation(0.1f * 0.75f, 0.1f * 0.25f);
  test_finished_summation(0.1f * 0.25f, 0.1f * 0.75f);
  test_finished_summation(0.1f * 0.33f, 0.1f * 0.67f);
  test_finished_summation(0.1f * 0.2f, 0.1f * 0.8f);
  test_finished_summation(0.0f, 0.0f);
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

void loop() {
  io::block_until_button_press();
  delay(100);

  auto data = measure_sh_gain(cluster, &DAQ, 4, 10);
  std::cout << "Read In = " << data << std::endl;
}

// both 0,9807
// only 1 0.4952
// only 0 0.4936
// none -0.0008