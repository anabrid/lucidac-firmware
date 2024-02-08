/*
 * NOTES
 * - FlexIODAQ can not be instantiated in top-level code outside of functions
 */

#include <core_pins.h>
#include <unity.h>

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

FlexIOControl mode_ctrl{};

void do_run(RunConfig run_config, DAQConfig daq_config) {
  TEST_ASSERT(daq_config.is_valid());

  std::string run_id{"550e8400-e29b-11d4-a716-446655440000"};
  Run run_{run_id, run_config, daq_config};

  TEST_ASSERT(FlexIOControl::init(run_config.ic_time, run_config.op_time));
  FlexIODAQ daq_{run_, daq_config, &dummy_run_data_handler};
  dummy_run_data_handler.num_of_data_vectors_streamed = 0;
  daq_.reset();
  TEST_ASSERT(daq_.init(0));
  daq_.enable();

  auto t_start = millis();
  FlexIOControl::force_start();

  while (!FlexIOControl::is_done()) {
    if(!daq_.stream()) {
      TEST_FAIL_MESSAGE("daq streaming error");
    }
    if ((millis() - t_start) > 2000ull * (run_config.ic_time + run_config.op_time)) {
      TEST_FAIL_MESSAGE("run is taking longer than it should");
    }
  }
  // When a data sample must be gathered very close to the end of OP duration,
  // it takes about 1 microseconds for it to end up in the DMA buffer.
  // This is hard to check for, since the DMA active flag is only set once the DMA
  // is triggered by the last CLK pulse, which is after around 1 microsecond.
  // Easiest solution is to wait for it.
  delayMicroseconds(1);
  if(!daq_.stream(true)) {
    TEST_FAIL_MESSAGE("daq remaining partial data streaming error");
  }

  // This should be unnecessary
  mode::FlexIOControl::to_end();

  // DAQ finalize clears some pointers and stuff
  daq_.finalize();
}

void one_time_setup() {}

void setUp() {
  // This is called before *each* test.
}

void tearDown() {
  // This is called after *each* test.
  FlexIOControl::reset();
}

void test_get_number_of_data_vectors_in_buffer_few() {
  do_run({1000, 42'000},{1, 100'000});
  TEST_ASSERT_EQUAL(5, ContinuousDAQ::get_number_of_data_vectors_in_buffer());
  TEST_ASSERT_EQUAL(5, dummy_run_data_handler.num_of_data_vectors_streamed);
}

void test_get_number_of_data_vectors_in_buffer_none() {
  // When buffer was filled exactly once
  TEST_ASSERT_EQUAL(dma::BUFFER_SIZE, 32*8);
  do_run({1000, 32*10'000},{8, 100'000});
  TEST_ASSERT_EQUAL(0, ContinuousDAQ::get_number_of_data_vectors_in_buffer());
  TEST_ASSERT_EQUAL(32, dummy_run_data_handler.num_of_data_vectors_streamed);
}

void test_get_number_of_data_vectors_in_buffer_one() {
  do_run({1000, 3500},{1, 100'000});
  TEST_ASSERT_EQUAL(1, ContinuousDAQ::get_number_of_data_vectors_in_buffer());
  TEST_ASSERT_EQUAL(1, dummy_run_data_handler.num_of_data_vectors_streamed);
}

void test_get_number_of_data_vectors_in_buffer_one_different() {
  do_run({1000, 42000},{1, 10'000});
  TEST_ASSERT_EQUAL(1, ContinuousDAQ::get_number_of_data_vectors_in_buffer());
  TEST_ASSERT_EQUAL(1, dummy_run_data_handler.num_of_data_vectors_streamed);
}

void test_get_number_of_data_vectors_in_buffer_two_very_close_timing() {
  // When last sample is taken basically exactly when OP ends
  do_run({1000, 10'000},{1, 100'000});
  TEST_ASSERT_EQUAL(2, ContinuousDAQ::get_number_of_data_vectors_in_buffer());
  TEST_ASSERT_EQUAL(2, dummy_run_data_handler.num_of_data_vectors_streamed);
}

void test_get_number_of_data_vectors_in_buffer_few_once_overflowed() {
  TEST_ASSERT_EQUAL(dma::BUFFER_SIZE, 32*8);
  do_run({1000, 33*10'000},{8, 100'000});
  TEST_ASSERT_EQUAL(1, ContinuousDAQ::get_number_of_data_vectors_in_buffer());
  TEST_ASSERT_EQUAL(33, dummy_run_data_handler.num_of_data_vectors_streamed);
}

void setup() {
  one_time_setup();
  delayMicroseconds(100);
  UNITY_BEGIN();
  RUN_TEST(test_get_number_of_data_vectors_in_buffer_few);
  RUN_TEST(test_get_number_of_data_vectors_in_buffer_none);
  RUN_TEST(test_get_number_of_data_vectors_in_buffer_one);
  RUN_TEST(test_get_number_of_data_vectors_in_buffer_one_different);
  RUN_TEST(test_get_number_of_data_vectors_in_buffer_two_very_close_timing);
  RUN_TEST(test_get_number_of_data_vectors_in_buffer_few_once_overflowed);
  UNITY_END();
}

void loop() {}