// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#include "carrier/cluster.h"
#include "daq/daq.h"
#include "mode/mode.h"

using namespace blocks;
using namespace daq;
using namespace platform;
using namespace mode;

Cluster cluster{};
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

  // Put cluster start-up sequence into a test case, so we can assert it worked.
  TEST_ASSERT(cluster.init());
  // Assert we have the necessary blocks
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.ublock, "U-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.cblock, "C-Block not inserted");
  TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.iblock, "I-Block not inserted");
  // TEST_ASSERT_NOT_EQUAL_MESSAGE(nullptr, cluster.m1block, "M1-Block not inserted");

  // Calibrate
  TEST_ASSERT(daq_.init(0));
  delayMicroseconds(50);
  TEST_ASSERT(cluster.calibrate(&daq_));
  delayMicroseconds(200);
}

void test_function() {
  auto *intblock = (MIntBlock *)(cluster.m1block);

  // We need a +1 later
  TEST_ASSERT(cluster.ublock->use_alt_signals(UBlock::ALT_SIGNAL_REF_HALF));

  // Choose integrators and set IC
  // An IC value's sign is actually equal to the output's sign (not inverted)
  struct monadic {
    uint8_t in, out;
  };

  struct dyadic {
    uint8_t a, b, out;
  };

  // integrator addresses
  monadic mx{MBlock::M1_INPUT(0), MBlock::M1_OUTPUT(0)}, y{MBlock::M1_INPUT(1), MBlock::M1_OUTPUT(1)},
      z{MBlock::M1_INPUT(2), MBlock::M1_OUTPUT(2)};

  // multiplier address
  dyadic mult{MBlock::M2_INPUT(0), MBlock::M2_INPUT(1), MBlock::M2_OUTPUT(0)};

  // Source of +1 Input (it is 1 after going throught the U block)
  auto one = UBlock::ALT_SIGNAL_REF_HALF_INPUT;

  TEST_ASSERT(intblock->set_ic(0, 0.066)); // 0.066)); // mx
  TEST_ASSERT(intblock->set_ic(1, 0.));    // y
  TEST_ASSERT(intblock->set_ic(2, 0.));    // z

  int poti = 0;

#define patchi(a, b, c, d) TEST_ASSERT(cluster.route(a, b, c, d))
#define patch(a, b, c) TEST_ASSERT(cluster.route(a, poti++, b, c))

  poti = 4; // first three potis for ADC out

  cluster.ublock->connect(mx.out, UBlock::OUTPUT_IDX_RANGE_TO_ADC(0));
  cluster.ublock->connect(y.out, UBlock::OUTPUT_IDX_RANGE_TO_ADC(1));
  cluster.ublock->connect(z.out, UBlock::OUTPUT_IDX_RANGE_TO_ADC(2));
  cluster.ublock->connect(mult.out, UBlock::OUTPUT_IDX_RANGE_TO_ADC(3));

  // the following lines define the Roessler attractor. They were checked so many
  // times that we are really sure they should do it. But se also
  // "sinusoidal_stereoids" for another attempt.
  patchi(mx.out, 4, -1.25f, y.in);
  patchi(mx.out, 5, -1.f, mult.a);
  patchi(one, 6, -0.3796f, mult.a);
  patchi(mult.out, 7, -15.f, z.in);
  patchi(one, 16, 0.005f, z.in);
  patchi(y.out, 17, 0.2f, y.in);
  patchi(y.out, 18, 0.8f, mx.in);
  patchi(z.out, 19, 1.f, mult.b);
  patchi(y.out, 20, 2.3f, mx.in);

  // some confusion with the ACL_OUTs, easy to solve actually.

  // this is wrong:
  cluster.ublock->connect(mx.out, UBlock::IDX_RANGE_TO_ACL_OUT(0));   // 8 oder 15
  cluster.ublock->connect(y.out, UBlock::IDX_RANGE_TO_ACL_OUT(1));    // 9 oder 14
  cluster.ublock->connect(z.out, UBlock::IDX_RANGE_TO_ACL_OUT(2));    // 10 oder 13
  cluster.ublock->connect(mult.out, UBlock::IDX_RANGE_TO_ACL_OUT(3)); // 11 oder 12

  /*
    // this is right:
    cluster.ublock->connect(mx.out,   UBlock::IDX_RANGE_TO_ACL_OUT(4)); // 8 oder 15
    cluster.ublock->connect(y.out,    UBlock::IDX_RANGE_TO_ACL_OUT(5)); // 9 oder 14
    cluster.ublock->connect(z.out,    UBlock::IDX_RANGE_TO_ACL_OUT(6)); // 10 oder 13
    cluster.ublock->connect(mult.out, UBlock::IDX_RANGE_TO_ACL_OUT(7)); // 11 oder 12
  */

  // This is another seperate test program:
  /*
    patch(one, 0.8f, mult.a);
    patch(one, 0.8f, mult.b);
  */

  /*
    cluster.ublock->connect(mult.out, UBlock::IDX_RANGE_TO_ACL_OUT(0));
    cluster.ublock->connect(mult.out, UBlock::IDX_RANGE_TO_ACL_OUT(7));
    //cluster.ublock->connect(mult.out, UBlock::OUTPUT_IDX_RANGE_TO_ADC(0)); // egal weil poti=0
    //cluster.ublock->connect(mult.out, UBlock::OUTPUT_IDX_RANGE_TO_ADC(0)));

    // readout

    cluster.ublock->connect(y.out,     UBlock::IDX_RANGE_TO_ACL_OUT(0));
    cluster.ublock->connect(z.out,     UBlock::IDX_RANGE_TO_ACL_OUT(1));
    cluster.ublock->connect(mult.out,  UBlock::IDX_RANGE_TO_ACL_OUT(2));
  */

  // Write to hardware
  cluster.write_to_hardware();
  delayMicroseconds(100);

  TEST_MESSAGE("Written to hardware, starting IC OP.");

  for (;;) {
    mode::ManualControl::to_ic();
    delayMicroseconds(150);

    mode::ManualControl::to_op();
    delayMicroseconds(5000);
  }

  /*
      TEST_MESSAGE("AQUISITION WITHIN IC MODE:");
      for(int i=0; i<4; i++) {
        String msg = String("daq[") + String(i) + String("] = ") + String(daq_.sample(i));
        TEST_MESSAGE(msg.c_str());
      }


    // Run it
  //  for(;;) {
    for(;;) {

    TEST_MESSAGE("QUICK IC OP CYCLE");
    for(;;) {

      mode::ManualControl::to_ic();
      delayMicroseconds(120);

      TEST_MESSAGE("AQUISITION WITHIN IC MODE:");
      for(int i=0; i<2; i++) {
        String msg = String("daq[") + String(i) + String("] = ") + String(daq_.sample(i));
        TEST_MESSAGE(msg.c_str());
      }

      delayMicroseconds(120);
      mode::ManualControl::to_op();
      delayMicroseconds(20000);
      mode::ManualControl::to_halt();
      delayMicroseconds(120);

    mode::ManualControl::to_op();
    delayMicroseconds(1000*6666);
    mode::ManualControl::to_halt();

    TEST_MESSAGE("CYCLE");
  */
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init);
  RUN_TEST(test_function);
  UNITY_END();
}

void loop() {}
