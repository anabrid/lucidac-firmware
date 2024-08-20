// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/*
 * NOTES
 * - FlexIODAQ can not be instantiated in top-level code outside of functions
 */

#include <Arduino.h>
#include <unity.h>

#include "test_parametrized.h"

// Include things with all private members made public,
// to inspect internal variables without additional hassle.
#define private public
#define protected public
#include "daq/daq.h"
#include "mode/mode.h"

using namespace daq;
using namespace mode;
using namespace run;

class DummyRunDataHandler : public RunDataHandler {
public:
  unsigned long num_of_data_vectors_streamed = 0;

  void handle(volatile uint32_t *data, size_t outer_count, size_t inner_count, const Run &run) override {
    num_of_data_vectors_streamed += outer_count;
  }

  void stream(volatile uint32_t *buffer, Run &run) override {}

  void prepare(Run &run) override {}
};

DummyRunDataHandler dummy_run_data_handler{};

void test_number_of_samples(RunConfig run_config, DAQConfig daq_config, size_t absolute_number_of_samples) {
  TEST_ASSERT(daq_config.is_valid());

  std::string run_id{"550e8400-e29b-11d4-a716-446655440000"};
  Run run_{run_id, run_config, daq_config};

  // Reset DAQ before initializing FlexIOControl, which currently produces
  // short low pulses on OP and triggers DAQ. That's not a problem, but it
  // is confusing when looking at the signals with the oscilloscope.
  FlexIODAQ daq_{run_, daq_config, &dummy_run_data_handler};
  dummy_run_data_handler.num_of_data_vectors_streamed = 0;
  daq_.reset();

  TEST_ASSERT(FlexIOControl::init(run_config.ic_time, run_config.op_time, mode::OnOverload::IGNORE,
                                  mode::OnExtHalt::IGNORE));
  TEST_ASSERT(daq_.init(0));
  daq_.enable();

  auto t_start = millis();
  FlexIOControl::force_start();

  while (!FlexIOControl::is_done()) {
    TEST_ASSERT_MESSAGE(daq_.stream(), "DAQ streaming error");
    if ((millis() - t_start) > 2 * 1000ull * (run_config.ic_time + run_config.op_time)) {
      TEST_FAIL_MESSAGE("run is taking longer than it should");
    }
  }
  // When a data sample must be gathered very close to the end of OP duration,
  // it takes about 1 microseconds for it to end up in the DMA buffer.
  // This is hard to check for, since the DMA active flag is only set once the DMA
  // is triggered by the last CLK pulse, which is after around 1 microsecond.
  // Easiest solution is to wait for it.
  delayMicroseconds(5);
  if (!daq_.stream(true)) {
    TEST_FAIL_MESSAGE("daq remaining partial data streaming error");
  }

  // This should be unnecessary
  mode::FlexIOControl::to_end();

  // DAQ finalize clears some pointers and stuff
  TEST_ASSERT(daq_.finalize());

  // Check number of absolute and last samples seen
  delayMicroseconds(5);
  TEST_ASSERT_EQUAL(absolute_number_of_samples, dummy_run_data_handler.num_of_data_vectors_streamed);
  TEST_ASSERT_EQUAL(absolute_number_of_samples % (dma::BUFFER_SIZE / daq_config.get_num_channels()),
                    ContinuousDAQ::get_number_of_data_vectors_in_buffer());
}

void setUp() {
  // This is called before *each* test.
  // The tests assume a certain buffer size
  TEST_ASSERT_EQUAL(dma::BUFFER_SIZE, 32 * 8);
}

void tearDown() {
  // This is called after *each* test.
  FlexIOControl::reset();
}

void setup() {
  delayMicroseconds(100);

  // CARE: Currently, the OP time is not absolutely perfect, so the number of samples
  //       is not exactly sample_rate*op_time and determined empirically.

  UNITY_BEGIN();
  // Some short ones and corner-cases
  RUN_PARAM_TEST(test_number_of_samples, {1000, 45'000}, {1, 100'000}, 5);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 32 * 10'000}, {8, 100'000}, 32);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 3500}, {1, 100'000}, 1);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 42000}, {1, 10'000}, 1);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 10'000}, {1, 100'000}, 2);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 33 * 10'000}, {8, 100'000}, 33);
  // Do something longer
  RUN_PARAM_TEST(test_number_of_samples, {1000, 785'000}, {8, 100'000}, 79);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 795'000}, {8, 100'000}, 80);
  RUN_PARAM_TEST(test_number_of_samples, {1000, 1'005'000}, {8, 100'000}, 100);
  UNITY_END();
}

void loop() {}
