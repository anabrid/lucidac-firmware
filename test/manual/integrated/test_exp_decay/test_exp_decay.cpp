// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "bus/bus.h"
#include "daq/daq.h"
#include "mode/mode.h"

#define private public
#include "block/cblock.h"
#include "block/iblock.h"
#include "block/mblock.h"
#include "block/ublock.h"

using namespace daq;
using namespace blocks;

OneshotDAQ DAQ{};
MIntBlock intblock{MBlock::SLOT::M1};
UBlock ublock;
// Replace by complete CBlock sometime :)
::functions::AD5452 mdac{bus::idx_to_addr(0, bus::C_BLOCK_IDX, 1)};
IBlock iblock;

void setUp() {
  // Set stuff up here
  DAQ.init(0);
  bus::init();
  mode::ManualControl::init();

  // Initialize blocks
  intblock.init();
  ublock.init();
  iblock.init();

  // Use DIO13 = LED for synchronization
  pinMode(LED_BUILTIN, OUTPUT);
}

void tearDown() {
  // clean stuff up here
}

void test_setup() {
  // TODO: Adapt to new Int Block addressing
  // TEST_ASSERT_EQUAL(bus::idx_to_addr(0, bus::M1_BLOCK_IDX, 1), intblock.f_ic_dac.address);
}

void test_exp_decay() {
  // Works, sets value in IC on BL_OUT0 in M2 slot
  // BUT only when you remove the first four BL_IN* BL_OUT* jumper after power-on?
  // NO, only sometimes? It worked a few times, now everything is broken x_X
  // Now it works again. Very fishy. I'll just commit everything and this DOES WORK, but only sometimes :)
  // Spoiler: Was a problem with the hardware,

  intblock.set_ic_value(0, +1.0f);
  intblock.write_to_hardware();
  // Signal does arrive at UBlock BL_IN0

  ublock.connect(0, 0);
  ublock.write_to_hardware();
  // Signal leaves UBlock on BL_OUT0, *inverted and doubled*

  // CBlock should do out = -in without configuration
  mdac.set_scale(+0.5f);

  // IBlock should connect (CBlock BL_OUT0 = IBlock BL_OUT0 (=input!)) to (M2 Block IN0 = IBlock MBL_IN0
  // (!kicad schematic is not up-to-date) = IBlock output 0)
  iblock.connect(0, 0);
  iblock.write_to_hardware();

  digitalWriteFast(LED_BUILTIN, HIGH);
  mode::ManualControl::to_ic();
  delayMicroseconds(120);
  mode::ManualControl::to_op();
  delayMicroseconds(666);
  mode::ManualControl::to_halt();
  digitalWriteFast(LED_BUILTIN, LOW);
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_setup);
  RUN_TEST(test_exp_decay);
  UNITY_END();
}

void loop() {}
