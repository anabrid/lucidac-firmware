// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define ANABRID_PEDANTIC

#include "daq/daq.h"
#include "io/io.h"
#include "lucidac/lucidac.h"

#include <iostream>

/**
 * This is a qualitative test.
 * 
 * Note that we have written Sinus tests MANY times. They even exist, right now, in
 * this repository! Have a look at
 * 
 * - manual/integrated/test_sinusoidal
 * - manual/integrated/test_identify_bus_output
 * 
 * How to use this test:
 * 
 * - Use LUCIDAC with M0 = M-Block-Int
 * - In order Connect MCX MANUALLY on Front panel:
 *   - Connect AUX0 to IN0
 *   - Connect AUX1 to IN1
 *
 **/

using namespace platform;

LUCIDAC lucidac;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init_and_blocks() {
  bus::init();
  io::init();
  mode::ManualControl::init();

  // In carrier_.init(), missing blocks are ignored
  TEST_ASSERT(lucidac.init());
  delayMicroseconds(50);

  lucidac.reset(false);
  // We do need certain blocks
  for (auto &cluster : lucidac.clusters) {
    TEST_ASSERT_NOT_NULL(cluster.ublock);
    TEST_ASSERT_NOT_NULL(cluster.cblock);
    TEST_ASSERT_NOT_NULL(cluster.iblock);
    //TEST_ASSERT_NOT_NULL(cluster.shblock);
  }
  TEST_ASSERT_NOT_NULL(lucidac.ctrl_block);
}

void test_sinus() {
  auto cluster = lucidac.clusters[0];

  // This assumes an M-Int-Block in slot M0
  // TODO: Rewrite that it can use any M-Int-Block
  auto mintblock = (blocks::MIntBlock*)cluster.m0block;
  TEST_ASSERT(mintblock != nullptr);
  TEST_ASSERT(mintblock->is_entity_type(blocks::MBlock::TYPES::M_INT8_BLOCK));

  // Determine which integrators to use
  uint8_t i0 = 2;
  uint8_t i1 = 3;

  TEST_ASSERT(mintblock->set_ic_value(i0, 1));
  TEST_ASSERT(mintblock->set_ic_value(i1, 0));

  // Which lanes to use
  uint8_t l0 = 2;
  uint8_t l1 = 3;

  TEST_ASSERT(cluster.route(i0, l0, +0.25, i1));
  TEST_ASSERT(cluster.route(i1, l1, -0.5,  i0));

  uint8_t acl_lane = 24;
  TEST_ASSERT(cluster.route(i0,   acl_lane, 1.0, 15));
  TEST_ASSERT(cluster.route(i1, ++acl_lane, 1.0, 15));

  TEST_ASSERT(cluster.write_to_hardware());
  delay(10);

  for(;;) {
    TEST_MESSAGE("loop");
    mode::ManualControl::to_ic();
    delayMicroseconds(120);
    mode::ManualControl::to_op();
    delayMicroseconds(5000);
  }
}


void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  RUN_TEST(test_sinus);
  UNITY_END();
}

void loop() { delay(500); }
