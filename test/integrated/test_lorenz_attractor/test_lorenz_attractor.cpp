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

#include "carrier/cluster.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace lucidac;
using namespace mode;

LUCIDAC luci{};
OneshotDAQ daq_{};

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
}

void test_init() {
  // Initialize mode controller (currently separate thing)
  ManualControl::init();

  // Put LUCIDAC start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(luci.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, luci.m1block, "M1-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(luci.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_function() {
  auto *intblock = (MIntBlock *)(luci.m1block);

  // We need a +1 later
  TEST_ASSERT(luci.ublock->use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));

  // Currently, not all coefficients are working :)
  uint8_t coeff_idx = 0;
  std::array<uint8_t, 13> coeffs{1,  2,  3,  4, /*7 is UBlock::ALT_SIGNAL_REF_HALF_INPUT,*/ 13, 15, 17,
                                 18, 19, 20, 21, 22};

  // Choose integrators and set IC
  // An IC value's sign is actually equal to the output's sign (not inverted)
  auto int_x = 0;
  auto int_y = 1;
  auto int_z = 2;
  TEST_ASSERT(intblock->set_ic(int_x, -1));
  TEST_ASSERT(intblock->set_ic(int_y, 0));
  TEST_ASSERT(intblock->set_ic(int_z, 0));

  // Integrator feedbacks
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_x), coeffs[coeff_idx++], -1.0f, MBlock::M1_INPUT(int_x)));
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_y), coeffs[coeff_idx++], -0.1f, MBlock::M1_INPUT(int_y)));
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_z), coeffs[coeff_idx++], -0.2667f, MBlock::M1_INPUT(int_z)));

  // Multiply x*y
  auto mul_xy = 0;
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_x), coeffs[coeff_idx++], 1.0f, MBlock::M2_INPUT(mul_xy*2)));
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_y), coeffs[coeff_idx++], 1.0f, MBlock::M2_INPUT(mul_xy*2 + 1)));
  // and scale and connect to Z integrator
  TEST_ASSERT(luci.route(MBlock::M2_OUTPUT(mul_xy), coeffs[coeff_idx++], -1.5f, MBlock::M1_INPUT(int_z)));

  // Scale Z and add -1 and multiply with x
  auto mul_xz = 1;
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_z), coeffs[coeff_idx++], 2.68f, MBlock::M2_INPUT(mul_xz*2)));
  TEST_ASSERT(luci.route(UBlock::ALT_SIGNAL_REF_HALF_INPUT, coeffs[coeff_idx++], 1.0f, MBlock::M2_INPUT(mul_xz*2)));
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_x), coeffs[coeff_idx++], 1.0f, MBlock::M2_INPUT(mul_xz*2+1)));

  // Add result to Y integrator input
  TEST_ASSERT(luci.route(MBlock::M2_OUTPUT(mul_xz), coeffs[coeff_idx++], -1.536, MBlock::M1_INPUT(int_y)));

  // And add y on x
  TEST_ASSERT(luci.route(MBlock::M1_OUTPUT(int_y), coeffs[coeff_idx++], 1.8, MBlock::M1_INPUT(int_x)));

  // Allow to read out integrator U block values on VG analog readout
  luci.ublock->connect(MBlock::M1_OUTPUT(int_x),  8);
  luci.ublock->connect(MBlock::M1_OUTPUT(int_y),  9);
  luci.ublock->connect(MBlock::M1_OUTPUT(int_z), 10);

  // Write to hardware
  luci.write_to_hardware();
  delayMicroseconds(100);

  TEST_MESSAGE("Written to hardware, starting IC OP.");

  // Run it
  for(;;) {
  
  mode::ManualControl::to_ic();
  delay(1);
  mode::ManualControl::to_op();
  delay(1000);
  mode::ManualControl::to_halt();

  TEST_MESSAGE("CYCLE");

  }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
