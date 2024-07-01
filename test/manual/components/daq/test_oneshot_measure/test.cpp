// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define protected public
#define private public

#include "block/blocks.h"
#include "carrier/carrier.h"

#include "daq/daq.h"

using namespace blocks;

carrier::Carrier carrier_({Cluster(0)});
auto &cluster = carrier_.clusters[0];

void init() {
  TEST_ASSERT(carrier_.init());
  Serial.println("Starting: ");

  cluster.shblock->set_track.trigger();
  delay(100);
  cluster.shblock->set_inject.trigger();
  delay(100);

  cluster.shblock->set_gain_channels_zero_to_seven.trigger();
  TEST_ASSERT(cluster.ublock->connect_alternative(UBlock::POS_BIG_REF, 0));
  TEST_ASSERT(cluster.cblock->set_factor(0, 1.0f));
  TEST_ASSERT(cluster.iblock->connect(0, 7));
  TEST_ASSERT(carrier_.write_to_hardware());
}

void setup() {
  bus::init();
  UNITY_BEGIN();

  RUN_TEST(init);

  UNITY_END();
}

void loop() {}
