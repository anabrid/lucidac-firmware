// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "io/io.h"

#define private public
#define protected public
#include "lucidac/lucidac.h"
#include "utils/logging.h"

using namespace platform;
using ACL = LUCIDAC_HAL::ACL;

LUCIDAC lucidac;

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_acl_prg() {
  TEST_ASSERT(lucidac.init());
  lucidac.reset(entities::ResetAction::CIRCUIT_RESET);

  // M0 = M-Block ID, instead of INT
  // M1 = M-Block MUL, as in normal LUCIDAC

  for (int i = 0; i < 32; i++)
    TEST_ASSERT(lucidac.clusters[0].cblock->set_factor(i, 1.0));
  TEST_ASSERT(lucidac.clusters[0].cblock->write_to_hardware());

  for (int i = 0; i < 8; i++) {
    TEST_ASSERT(lucidac.clusters[0].route(8 + i, 24 + i, 0.5, 8 + i));

    // TEST_ASSERT(lucidac.clusters[0].add_constant(blocks::UBlock::Transmission_Mode::POS_REF, 24+i, 0.1, i));

    TEST_ASSERT(lucidac.set_acl_select(i, ACL::EXTERNAL_));
  }

  TEST_ASSERT(lucidac.write_to_hardware());
}

void setup() {
  bus::init();
  io::init();

  msg::Log::get().sinks.add(&Serial);

  UNITY_BEGIN();
  RUN_TEST(test_acl_prg);
  UNITY_END();
}

void loop() { delay(100); }
