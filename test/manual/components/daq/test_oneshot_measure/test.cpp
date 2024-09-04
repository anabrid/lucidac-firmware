// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <iostream>
#include <unity.h>

#define protected public
#define private public

#include "block/blocks.h"
#include "lucidac/lucidac.h"

#include "daq/daq.h"

using namespace blocks;
using namespace entities;

platform::LUCIDAC lucidac_;
auto &cluster = lucidac_.clusters[0];
daq::OneshotDAQ DAQ;

void init() {
  TEST_ASSERT(lucidac_.init());
  Serial.println("Starting: ");

  cluster.shblock->compensate_hardware_offsets();

  cluster.shblock->set_state(SHBlock::State::GAIN_ZERO_TO_SEVEN);

  TEST_ASSERT(cluster.ublock->connect_alternative(UBlock::Transmission_Mode::POS_REF, 0));
  TEST_ASSERT(cluster.cblock->set_factor(0, 0.0f));
  TEST_ASSERT(cluster.iblock->connect(0, 0));
  TEST_ASSERT(cluster.write_to_hardware());
}

void setup() {
  bus::init();
  DAQ.init(0);
  UNITY_BEGIN();

  pinMode(29, OUTPUT);

  digitalWrite(29, LOW);
  delay(1000);

  RUN_TEST(init);

  delay(1000);

  digitalWrite(29, HIGH);
  delay(10);
  digitalWrite(29, LOW);

  std::cout << std::fixed;
}

float coeff = -1.0f;

void loop() {
  cluster.cblock->set_factor(0, coeff);
  cluster.cblock->write_to_hardware();

  // std::cout << millis() << "," << coeff << "," << DAQ.sample_raw(0) << std::endl;
  std::cout << millis() << "," << coeff << "," << DAQ.sample_avg_raw(5, 100)[0] << std::endl;

  delay(50);

  coeff += 0.002f;

  if (coeff > 1.0f)
    while (1)
      ;
}
