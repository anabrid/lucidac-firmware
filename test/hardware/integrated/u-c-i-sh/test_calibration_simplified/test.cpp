// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "test_fmtlib.h"
#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public
#include "daq/daq.h"
#include "io/io.h"

#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

LUCIDAC carrier_;
auto &cluster_ = carrier_.clusters[0];
blocks::CTRLBlockHAL_V_1_0_2 ctrlblock_hal;
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
  TEST_ASSERT_NOT_NULL(cluster_.ublock);
  TEST_ASSERT_NOT_NULL(cluster_.cblock);
  TEST_ASSERT_NOT_NULL(cluster_.iblock);
  TEST_ASSERT_NOT_NULL(cluster_.shblock);
}

void test_calibration() {
  // Connect ADC_BUS on CTRL-Block
  // TODO: This should actually happen inside cluster_.calibrate, but I forgot :)
  ctrlblock_hal.write_adc_bus_muxers(blocks::CTRLBlock::ADCBus::CL0_GAIN);

  // The calibration expects that some connections are configured,
  // otherwise it does not do anything (it skips unconnected lanes).
  TEST_ASSERT(cluster_.route(2, 0, 0.3f, 1));
  TEST_ASSERT(cluster_.route(1, 1, -0.2f, 2));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 3, 0.114f, 1));

  // Actually write configuration to hardware
  TEST_ASSERT(cluster_.write_to_hardware());

  // Do the calibration
  // See https://lab.analogparadigm.com/groups/lucidac/-/epics/14
  // and comments in the calibrate function for more information
  TEST_ASSERT(cluster_.calibrate(&DAQ));

  // Check whether all gain corrections are in a reasonable range
  TEST_MESSAGE_FORMAT("Gain corrections are {}.", cluster_.cblock->get_gain_corrections());
  for (auto gain_correction : cluster_.cblock->get_gain_corrections())
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 1.0f, gain_correction);

  // Here you can do whatever you want after calibration is finished.
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
