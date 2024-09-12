// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "io/io.h"

#define private public
#define protected public
#include "block/ctrlblock.h"

using namespace blocks;

CTRLBlockHAL_V_1_0_2 hal;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_adc_mux() {
  TEST_ASSERT(hal.write_adc_bus_muxers(CTRLBlock::ADCBus::CL0_GAIN));
  io::block_until_button_press_and_release();
  //TEST_ASSERT(hal.write_adc_bus_muxers(CTRLBlock::ADCBusSelect::CL1_GAIN));
  //io::block_until_button_press_and_release();
  TEST_ASSERT(hal.write_adc_bus_muxers(CTRLBlock::ADCBus::CL2_GAIN));
  io::block_until_button_press_and_release();
  TEST_ASSERT(hal.write_adc_bus_muxers(CTRLBlock::ADCBus::ADC));
  io::block_until_button_press_and_release();
}

void setup() {
  io::init();
  bus::init();
  pinMode(LED_BUILTIN, OUTPUT);

  UNITY_BEGIN();
  RUN_TEST(test_adc_mux);
  UNITY_END();
}

void loop() {
  digitalToggleFast(LED_BUILTIN);
  //test_ctrl_block_hardware();
  delay(500);
}
