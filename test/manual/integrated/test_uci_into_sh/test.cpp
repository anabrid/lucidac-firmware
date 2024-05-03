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
  //SET UBLOCK OUTPUT TO +2V
  ublock.change_transmission_mode(UBlock::Transmission_Mode::POS_BIG_REF);

  for (auto output : UBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(ublock.connect(output / 2, output));
  }
  ublock.write_to_hardware();
}

void configure_iblock() {
  //Set I Block to n to n connection
  for (auto output : IBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(iblock.connect(output, output));
  }
  iblock.write_to_hardware();
}

void setup() {
  bus::init();
  pinMode(29, INPUT_PULLUP);

  UNITY_BEGIN();
  // RUN_TEST(test_init);
  RUN_TEST(configure_ublock);
  RUN_TEST(configure_iblock);

  shblock.set_track.trigger();

  UNITY_END();
}

void loop() {
  // wait till button is pressed
  while (digitalReadFast(29)) {
  }

  //output 100mV
  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * 0.1f));
  }
  cblock.write_to_hardware();

  delay(10); 
  shblock.set_track.trigger();
  delay(1000);
  shblock.set_inject.trigger();
  delay(10);


  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * 1.1f));
  }
  cblock.write_to_hardware();

  delay(60000);


  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * 0.1f));
  }
  cblock.write_to_hardware();


  while (digitalReadFast(29)) {
  }

  delay(10);
  shblock.set_track.trigger();
  delay(1000);
  shblock.set_inject.trigger();
  delay(10);

  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * -0.9f));
  }
  cblock.write_to_hardware();

  delay(60000);

  for (auto output : CBlock::OUTPUT_IDX_RANGE()) {
    TEST_ASSERT(cblock.set_factor(output, 0.5f * 0.1f));
  }
  cblock.write_to_hardware();
}
