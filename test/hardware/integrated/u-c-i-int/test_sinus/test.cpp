// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define ANABRID_PEDANTIC

#include "daq/daq.h"
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

void test_sinus() {
  auto cluster = lucidac.clusters[0];

  // TODO: Detect where we have the M-Int block readily available at the correct position.
  // This assumes an M-Int-Block in slot M0

  // Determine which integrators to use
  uint8_t i0 = 0;
  uint8_t i1 = 1;

  // Which lanes to use
  uint8_t l0 = 0;
  uint8_t l1 = 1;

  TEST_ASSERT(cluster.route(i0, l0, +0.25, i1));
  TEST_ASSERT(cluster.route(i1, l1, -0.5,  i0));

  // which ACL_OUT to use
  uint8_t out0 = 0;
  uint8_t out1 = 1;

  const uint8_t acl_lane_start = 24;

  TEST_ASSERT(lucidac.set_acl_select(out0, platform::LUCIDAC::ACL::EXTERNAL_));
  TEST_ASSERT(lucidac.set_acl_select(out1, platform::LUCIDAC::ACL::EXTERNAL_));

  // a cross-lane where to sink the ACL_IN to. We don't use it so
  // just put it so some computing element on M1 and hope for the best.
  // If you want to improve it, configure U/C block instead of using route()
  // in the next linkes.
  uint8_t sink = 15;

  TEST_ASSERT(cluster.route(i0, acl_lane_start + out0, 1.0, sink));
  TEST_ASSERT(cluster.route(i1, acl_lane_start + out1, 1.0, sink));

  TEST_ASSERT_EQUAL(1, cluster.write_to_hardware());
  delay(10);

  for (;;) {
    mode::ManualControl::to_ic();
    delayMicroseconds(120);
    mode::ManualControl::to_op();
    delayMicroseconds(5'000);
    mode::ManualControl::to_halt();
  }

}


void setup() {
  UNITY_BEGIN();

  bus::init();
  TEST_ASSERT(lucidac.init());

  lucidac.reset(false);
  RUN_TEST(test_sinus);

  UNITY_END();
}

void loop() { delay(500); }
