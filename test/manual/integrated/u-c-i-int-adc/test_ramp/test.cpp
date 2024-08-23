// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#ifndef ANABRID_PEDANTIC
#error "This test requires pedantic mode."
#endif

#include "block/mblock.h"
#include "daq/daq.h"
#include "io/io.h"
#include "lucidac/lucidac.h"

#include <iostream>

/**
 * This is an automatic test which makes use of the ADC.
 * It integrates ramps and checks for the final result with single shot DAQ.
 *
 * Note we have written this kind of tests MANY times. Maybe look at
 * the old versions, see also https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/issues/100
 *
 * How to use this test:
 *
 * - Use LUCIDAC with M0 = M-Block-Int
 *
 **/

using namespace platform;

LUCIDAC lucidac;
daq::OneshotDAQ the_daq;

uint8_t integrator, lane, adc_channel;

#define SHOW(variable) " " #variable "=" << variable

float slope, ic;
unsigned int time_factor; // k0
float op_time_us;
float expected_value;

void test_init_and_blocks() {
  bus::init();
  io::init();
  mode::ManualControl::init();

  // In carrier_.init(), missing blocks are ignored
  TEST_ASSERT(lucidac.init());
  delayMicroseconds(50);

  TEST_ASSERT(the_daq.init(0));

  lucidac.reset(false);
  // We do need certain blocks
  for (auto &cluster : lucidac.clusters) {
    TEST_ASSERT_NOT_NULL(cluster.ublock);
    TEST_ASSERT_NOT_NULL(cluster.cblock);
    TEST_ASSERT_NOT_NULL(cluster.iblock);
    // TEST_ASSERT_NOT_NULL(cluster.shblock);
  }
  TEST_ASSERT_NOT_NULL(lucidac.ctrl_block);
}

void configure() {
  std::cout << "A\n" << std::endl;
  lucidac.reset(false);
  std::cout << "B\n" << std::endl;
  auto cluster = lucidac.clusters[0];

  std::cout << "C\n" << std::endl;
  auto mintblock = (blocks::MIntBlock *)cluster.m0block;
  TEST_ASSERT(mintblock != nullptr);
  std::cout << "D\n" << std::endl;
  TEST_ASSERT(mintblock->is_entity_type(blocks::MBlock::TYPES::M_INT8_BLOCK));

  std::cout << "E\n" << std::endl;
  TEST_ASSERT(mintblock->set_ic_value(integrator, ic));
  TEST_ASSERT(mintblock->set_time_factor(integrator, time_factor));

  // Feed a constant into the integrator input
  std::cout << "F\n" << std::endl;
  TEST_ASSERT(cluster.add_constant(blocks::UBlock::Transmission_Mode::POS_REF, lane, slope, integrator));

  // ACL_OUT0
  uint8_t sink = 15;
  TEST_ASSERT(cluster.route(integrator, 24, 1.0, sink));

  TEST_ASSERT(lucidac.set_adc_channel(adc_channel, integrator));

  // default value, was set by lucidac.reset().
  TEST_ASSERT(lucidac.ctrl_block->get_adc_bus() == blocks::CTRLBlock::ADCBus::ADC);

  std::cout << "G\n" << std::endl;
  TEST_ASSERT(cluster.write_to_hardware());
  delay(10);
}

void run_and_check() {
  std::cout << "IC\n" << std::endl;
  mode::ManualControl::to_ic();
  delayMicroseconds(120); // probably shorten!
  std::cout << "OP\n" << std::endl;
  mode::ManualControl::to_op();
  delayMicroseconds(op_time_us);
  mode::ManualControl::to_halt();

  std::cout << "SAMPLE\n" << std::endl;
  uint16_t measured_value_raw = the_daq.sample_raw(adc_channel);
  std::cout << SHOW(measured_value_raw) << std::endl;
  float measured_value = the_daq.sample(adc_channel);
  std::cout << SHOW(measured_value) << std::endl;

  std::cout << SHOW(integrator) << SHOW(measured_value_raw) << SHOW(measured_value) << std::endl;

  float acceptable_abs_error = 0.1;

  TEST_ASSERT(fabs(measured_value - expected_value) < acceptable_abs_error);
}

int sign(float x) { return (x > 0) ? 1 : ((x < 0) ? -1 : 0); }

void all_testcases() {
  float slopes[] = {-1, +1, +10, -10};
  float ics[] = {+1, -1};
  unsigned int time_factors[] = {100, 10'000};

  integrator = 0;
  lane = 0;

  for (auto cur_slope : slopes)
    for (auto cur_ic : ics)
      for (auto cur_time_factor : time_factors) {
        slope = cur_slope;
        ic = cur_ic;
        time_factor = cur_time_factor;

        if (sign(slope) == sign(ic))
          continue;

        expected_value = -ic;

        // determine the required time until the ramp reaches expected_value.
        op_time_us = abs(expected_value - ic) / (abs(slope) * time_factor) * 1000 * 1000;

        std::cout << SHOW(slope) << SHOW(ic) << SHOW(time_factor) << SHOW(op_time_us) << std::endl;

        TEST_ASSERT(op_time_us > 0);
        RUN_TEST(configure);
        std::cout << "H\n" << std::endl;
        RUN_TEST(run_and_check);
      }
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_init_and_blocks);
  all_testcases(); // calls RUN_TEST internally
  UNITY_END();
}

void loop() { delay(500); }
