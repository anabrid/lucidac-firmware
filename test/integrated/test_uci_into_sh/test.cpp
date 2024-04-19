// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define private public
#define protected public
#include "block/blocks.h"

using namespace blocks;

UBlock ublock{0};
CBlock cblock{0};
IBlock iblock{0};
SHBlock shblock{0};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void configure_ublock() {
  ublock.changeTransmissionMode(UBlock_Transmission_Mode::SMALL_REF, false);
  ublock.changeTransmissionMode(UBlock_Transmission_Mode::SMALL_REF, false, true);

  for (auto output: UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(ublock.connect(output/2, output));
  }
  ublock.write_to_hardware();
}

void configure_cblock() {
  for (auto output: UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f));
  }
  cblock.write_to_hardware();
}

void configure_iblock() {
  for (auto output: IBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(iblock.connect(output, output));
  }
  iblock.write_to_hardware();
}

void configure_shblock() {
  shblock.set_inject.trigger();
}


void test_shblock() {
  shblock.set_track.trigger();
  delay(500);
  shblock.set_inject.trigger();
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();
  // RUN_TEST(test_init);
  RUN_TEST(configure_ublock);
  RUN_TEST(configure_cblock);
  RUN_TEST(configure_iblock);
  RUN_TEST(configure_shblock);
  UNITY_END();
}

void loop() {
  // Do an action once the button is pressed
  while (digitalReadFast(29)) {
  }
  shblock.set_track.trigger();
  delay(500);
  // Do an action once the button is pressed
  while (digitalReadFast(29)) {
  }
  shblock.set_inject.trigger();
  delay(500);
}
