// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>
#include <unity.h>

#include "daq.h"
#include "local_bus.h"
#include "mode.h"

#define private public
#include "mblock.h"
#include "ublock.h"
#include "iblock.h"

using namespace daq;
using namespace blocks;

OneshotDAQ DAQ{};
MIntBlock intblock{0, blocks::MIntBlock::M2_IDX};
UBlock ublock{0};
IBlock iblock{0};

void setUp() {
  // Set stuff up here
  DAQ.init();
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

void test_setup() { TEST_ASSERT_EQUAL(bus::idx_to_addr(0, bus::M2_BLOCK_IDX, 1), intblock.f_ic_dac.address); }

void test_exp_decay() {
  // Works, sets value in IC on BL_OUT0 in M2 slot
  // BUT only when you remove the first four BL_IN* BL_OUT* jumper after power-on?
  // NO, only sometimes? It worked a few times, now everything is broken x_X
  // Now it works again. Very fishy. I'll just commit everything and this DOES WORK, but only sometimes :)
  // Spoiler: Was a problem with the hardware,

  intblock.set_ic(0, +0.77);
  intblock.write_to_hardware();
  // Signal does arrive at UBlock BL_IN0

  ublock.connect(0,0);
  ublock.write_to_hardware();
  // Signal leaves UBlock on BL_OUT0, *inverted and doubled*

  // CBlock should do out = -in without configuration

  // IBlock should connect (CBlock BL_OUT0 = IBlock BL_OUT0 (=input!)) to (M2 Block IN0 = IBlock MBL_IN0 (!kicad schematic is not up-to-date) = IBlock output 0)
  iblock.outputs[0] = IBlock::INPUT_BITMASK(0);
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
