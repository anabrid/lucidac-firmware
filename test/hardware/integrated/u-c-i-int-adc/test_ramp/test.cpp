// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <unity.h>

#define ANABRID_PEDANTIC

#include "daq/daq.h"
#include "lucidac/lucidac.h"
#include "block/mblock.h"

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

uint8_t integrator, lane, const_lane, adc_channel;

#define SHOW(variable) " " #variable "=" << variable

float slope, ic;
unsigned int time_factor; // k0
float op_time_secs;
float expected_value = 1.0;

void setup_adc() {
  // setup the ADC
  adc_channel = 0;
  lucidac.set_adc_channel(adc_channel, integrator);

  // default value, was set by lucidac.reset().
  TEST_ASSERT(lucidac.ctrl_block->get_adc_bus() == blocks::CTRLBlock::ADCBus::ADC);

  TEST_ASSERT(the_daq.init(0));
}


void configure() {
    lucidac.reset(false);
    auto cluster = lucidac.clusters[0];

    // TODO: Detect where we have the M-Int block readily available at the correct position.
    // This assumes an M-Int-Block in slot M0

    // Determine which integrator to use
    integrator = 0;

    ic = -1.0;
    slope = 0.1;

    auto mintblock = (blocks::MIntBlock*)cluster.m0block;
    TEST_ASSERT(mintblock != nullptr);
    TEST_ASSERT(mintblock->set_ic_value(integrator, ic));
    TEST_ASSERT(mintblock->set_time_factor(integrator, time_factor));

    // Make up the constant
    const_lane = 0;
    if(lane == const_lane) const_lane++;

    // Feed a constant into the integrator input
    TEST_ASSERT(cluster.add_constant(blocks::UBlock::Transmission_Mode::POS_REF, const_lane, slope, integrator));

    bool use_acl_out_for_debugging = true;
    if(use_acl_out_for_debugging) {
        // which ACL_OUT to use
        uint8_t acl_out = 0;
        const uint8_t acl_lane_start = 24;
        TEST_ASSERT(lucidac.set_acl_select(acl_out, platform::LUCIDAC::ACL::EXTERNAL_));

        // a cross-lane where to sink the ACL_IN to. We don't use it so
        // just put it so some computing element on M1 and hope for the best.
        // If you want to improve it, configure U/C block instead of using route()
        // in the next linkes.
        uint8_t sink = 15;

        TEST_ASSERT(cluster.route(integrator, acl_lane_start + acl_out, 1.0, sink));
    }

    TEST_ASSERT_EQUAL(1, cluster.write_to_hardware());
    delay(10);
}

void run_and_check() {
    mode::ManualControl::to_ic();
    delayMicroseconds(120); // probably shorten!
    mode::ManualControl::to_op();
    
    delayMicroseconds((int)(op_time_secs * 1000));

    mode::ManualControl::to_halt();

    float measured_value_raw = the_daq.sample_raw(adc_channel);
    float measured_value = the_daq.sample(adc_channel);

    std::cout << SHOW(integrator) << SHOW(measured_value_raw) << SHOW(measured_value) << std::endl;

    float acceptable_abs_error = 0.1;

    TEST_ASSERT( fabs(measured_value - expected_value) < acceptable_abs_error );
}

void all_testcases() {
    float slopes[] = { -1, +1};
    float ics[]    = { +1, -1};

    unsigned int time_factors[] = { 100, 10'000 };

    for(int conf_idx=0; conf_idx < sizeof(slopes) / sizeof(slopes[0]); conf_idx++) {
        slope = slopes[conf_idx];
        ic = ics[conf_idx];

        for(int factor_idx; factor_idx < sizeof(time_factors) / sizeof(time_factors[0]); factor_idx++) {
            time_factor = time_factors[factor_idx];

            // determine the required time until the ramp reaches expected_value.
            op_time_secs = (expected_value / time_factor - ic) / slope;

            std::cout << SHOW(slope) << SHOW(ic) << SHOW(time_factor) << SHOW( op_time_secs ) << std::endl;

            RUN_TEST(setup);
            RUN_TEST(run_and_check);
        }
    }
}

void setup() {
  UNITY_BEGIN();

  bus::init();
  TEST_ASSERT(lucidac.init());

  RUN_TEST(setup_adc);
  all_testcases(); // calls RUN_TEST internally

  UNITY_END();
}

void loop() { delay(500); }
