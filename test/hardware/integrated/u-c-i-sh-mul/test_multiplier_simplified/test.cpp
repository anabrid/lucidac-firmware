// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>
#include <iostream>

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
  // Require multiplier in second M-block slot
  // TODO: Multiplier actually needs EEPROM data
  // TEST_ASSERT_NOT_NULL(cluster_.m1block);
}

void test_calibration() {
  // Connect CL0_GAIN to ADC on CTRL-Block (this will immediately write to hardware)
  // TODO: This should actually happen inside cluster_.calibrate, but I forgot :)
  ctrlblock_hal.write_adc_bus_muxers(blocks::CTRLBlock::ADCBus::CL0_GAIN);

  // Route a set of constants to the multiplier
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 0, 0.1, 8 + 0));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 1, 0.2, 8 + 1));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 2, 0.3, 8 + 2));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 3, 0.4, 8 + 3));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 4, 0.5, 8 + 4));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 5, 0.6, 8 + 5));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 6, 0.7, 8 + 6));
  TEST_ASSERT(cluster_.add_constant(UBlock::Transmission_Mode::POS_REF, 7, 0.8, 8 + 7));
  // Actually write configuration to hardware
  TEST_ASSERT(cluster_.write_to_hardware());

  // Do the calibration
  // See https://lab.analogparadigm.com/groups/lucidac/-/epics/14
  // and comments in the calibrate function for more information
  TEST_ASSERT(cluster_.calibrate(&DAQ));

  // Here you can do whatever you want after calibration is finished.
  // E.g. set-up ADC measurements

  // Connect M-block outputs to ADC on CTRL-block (this will immediately write to hardware)
  // CARE: !!!
  // CARE: This will only work if the CTRL-block has the necessary hardware on it
  // CARE: !!!
  ctrlblock_hal.write_adc_bus_muxers(blocks::CTRLBlock::ADCBus::ADC);
  // Select M-block outputs on the carrier board
  // This connects M-block outputs 8-15 (second M-block slot) to ADC[0-7]
  // (this will immediately write to hardware)
  carrier_.hardware->write_adc_bus_mux({8, 9, 10, 11, 12, 13, 14, 15});

  delay(10);
  auto data = DAQ.sample_avg_raw(10, 100);
  std::cout << "DAQ values are " << data << std::endl;
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
