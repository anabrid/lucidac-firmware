// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "daq/daq.h"
#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"

using namespace platform;
using namespace blocks;

// TODO: Make this independent on underlying hardware by dynamically detecting carrier board
LUCIDAC carrier_;
auto &cluster = carrier_.clusters[0];

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test() {
  cluster.shblock->set_track.trigger();
  delay(100);
  cluster.shblock->set_inject.trigger();

  for (int i = 0; i < 32; i++) {
    TEST_ASSERT(cluster.ublock->connect_alternative(UBlock::Transmission_Mode::POS_SMALL_REF, i));
  }

  for (int i = 0; i < 32; i++)
    TEST_ASSERT(cluster.cblock->set_factor(i, 1.0f));

  for (int i = 3; i < 16; i++)
    TEST_ASSERT(cluster.iblock->connect(i, i));

  cluster.cblock->set_factor(2, -1.0f);
  cluster.iblock->connect(1, 1);
  cluster.iblock->connect(2, 1);

  std::bitset<IBlock::NUM_INPUTS> factors;
  factors.set();
  factors[2] = false;
  Serial.println(factors.to_string().c_str());
  cluster.iblock->set_upscaling(factors);

  TEST_ASSERT(cluster.write_to_hardware());
}

void setup() {
  bus::init();
  io::init();

  UNITY_BEGIN();

  TEST_ASSERT(carrier_.init());

  RUN_TEST(test);
  UNITY_END();
}

void loop() { delay(500); }
