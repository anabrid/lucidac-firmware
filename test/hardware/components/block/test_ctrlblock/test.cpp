// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "block/ctrlblock.h"

using namespace blocks;

CTRLBlock *ctrl_block;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void init() {
  ctrl_block = entities::detect<blocks::CTRLBlock>(bus::address_from_tuple(bus::CTRL_BLOCK_BADDR, 0));
  TEST_ASSERT_NOT_NULL(ctrl_block);
  TEST_ASSERT(ctrl_block->init());
}

void test_adc_mux() {
  TEST_ASSERT(ctrl_block->hardware->write_adc_bus_muxers(CTRLBlock::ADCBus::CL0_GAIN));
  TEST_ASSERT(ctrl_block->hardware->write_adc_bus_muxers(CTRLBlock::ADCBus::CL1_GAIN));
  TEST_ASSERT(ctrl_block->hardware->write_adc_bus_muxers(CTRLBlock::ADCBus::CL2_GAIN));
  TEST_ASSERT(ctrl_block->hardware->write_adc_bus_muxers(CTRLBlock::ADCBus::ADC));
}

void test_sync() {
  TEST_ASSERT(ctrl_block->hardware->write_sync_id(0b0'010101'0));
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();
  RUN_TEST(init);
  RUN_TEST(test_adc_mux);
  RUN_TEST(test_sync);
  UNITY_END();
}

void loop() { delay(500); }
