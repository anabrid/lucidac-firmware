// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "run/run_manager.h"

#include <cstdlib>
#include <cmath>
#include <Arduino.h>

#include "daq/daq.h"
#include "utils/logging.h"

// This is an interim hacky solution to introduce another kind of RunDataHandler
#include "protocol/protocol_oob.h"

run::RunManager run::RunManager::_instance{};

FLASHMEM
void run::RunManager::run_next(carrier::Carrier &carrier_, run::RunStateChangeHandler *state_change_handler,
                               run::RunDataHandler *run_data_handler,
                               client::StreamingRunDataNotificationHandler *alt_run_data_handler) {
  // TODO: Improve handling of queue, especially the queue.pop() later.
  auto run = queue.front();

  if(run.config.calibrate) {
    // TODO: In principle, we could do this slightly more efficient when we initialize the DAQ
    //       for the runs anyway, but the FlexIODAQ currently does not support a simple sample().
    LOG_ALWAYS("Calibrating routes for run...")
    daq::OneshotDAQ daq_;
    daq_.init(0);
    if (!carrier_.calibrate_routes(&daq_))
      LOG_ERROR("Error during self-calibration. Machine will continue with reduced accuracy.");
  }

  if(run.config.streaming)
    run_next_flexio(run, state_change_handler, run_data_handler);
  else
    run_next_traditional(run, state_change_handler, run_data_handler, alt_run_data_handler);

  if(!run.config.repetitive)
    queue.pop();
}

// NOT FLASHMEM
void run::RunManager::run_next_traditional(run::Run &run, RunStateChangeHandler *state_change_handler, RunDataHandler *run_data_handler, client::StreamingRunDataNotificationHandler *alt_run_data_handler) {
  //run_data_handler->prepare(run);

  daq::OneshotDAQ daq;


  // here, one sample is one data aquisition which always yields all channels.
  constexpr uint32_t max_num_samples = 16'384;
  const int num_channels = run.daq_config.get_num_channels();
  const uint32_t optime_us = run.config.op_time / 1000;

  uint32_t sampling_time_us = 0;
  if(num_channels) {
    daq.init(0);

    // measure how long an single ADC sampling takes with the current implementation.
    // This is right now around 15us.
    elapsedMicros sampling_time_us_counter; // class from teensy elpasedMillis.h
    daq.sample_raw();
    sampling_time_us = sampling_time_us_counter;
    // TODO: There is a slight dither around 15us -- 16us, could do the averaging call to
    //       improve the statistics.
  }

  /*
    The guiding principle of the following math is this:

       sampling_total_us == sampling_time_us + sampling_sleep_us // for up to full 8 channels
       optime_us == num_samples*num_channels*sampling_total_us

    We precompute all busy waits (sleeps) in order to match the requested optime as precise
    as possible, with this means. Without interrupts.

    We ignore run.daq_config.get_sample_rate() because it is much simpler to just always
    fill up the buffer. Furthermore the manual DAQ is pretty slow (about 15us per sample).

    There are two main scenarios:

    - asked for fast sampling = short runtime. This is where sampling_time_us dominates.
      This code does it's job when optime_us > sampling_time_us but it's not great, of
      course.
    - asked for slow sampling = long runtime. This is where this code shines, because there
      is no limit in runtime or similar, compared to the FlexIO code.

    Of course, the overall code is still an order of magnitude simpler then the FlexIO/DMA
    code. And in the same way less precise for short runtimes and fast sampling times.

    Given the unit of time basically measured in sampling_time_us, there is no need to
    compute in nanoseconds here. Therefore the code computes in microseconds.
  */

  uint32_t sampling_total_us = optime_us / max_num_samples;

  // either no sleep at all or sleep the difference between total time and aquisition time.
  uint32_t sampling_sleep_us = (sampling_total_us < sampling_time_us) ? 0 : (sampling_total_us - sampling_time_us);

  uint32_t num_samples       = optime_us / (sampling_sleep_us + sampling_time_us);
  uint32_t optime_left_us    = optime_us % (sampling_sleep_us + sampling_time_us);

  // This situation actually should not happen at all.
  if(num_samples > max_num_samples) num_samples = max_num_samples;

  const int num_buffer_entries = num_samples * num_channels;
  if(num_channels)
    LOGMEV("optime_us=%d, sampling_time_us=%d, sampling_sleep_us=%d, num_samples=%d, num_channels=%d", optime_us, sampling_time_us, sampling_sleep_us, num_samples, num_channels);

  // just to be able to use daq.sample_raw(uint16_t*) instead of copying over to an intermediate buffer
  // of size std::vector, we add a bit safety padding to the right for the last datum obtained.
  // Actually needed only if num_samples < daq::NUM_CHANNELS = 8.
  constexpr int padding = daq::NUM_CHANNELS;

  // we use malloc because new[] raises an exception if allocation fails but we cannot catch it with -fno-exception.
  // we don't use the stack because we expect the heap to have more memory left...
  uint16_t *buffer = nullptr;
  if(num_channels) {
    // allocate only if any data aquisition was asked for
    buffer = (uint16_t*) malloc((num_buffer_entries + padding) * sizeof(uint16_t));
    if(!buffer) {
      LOG_ERROR("Could not allocated a large run buffer for a traditional Run.");
    }
  }

  mode::RealManualControl::enable();

  LOGMEV("IC TIME: %lld", run.config.ic_time);
  LOGMEV("OP TIME: %lld", run.config.op_time);

  mode::RealManualControl::to_ic();
  // 32bit nanosecond delay can sleep maximum 4sec, however
  // an overlap will happen instead.
  if(run.config.ic_time > 100'000'000) // 100ms
    delay(run.config.ic_time / 1'000'000); // milisecond resolution sleep
  else if(run.config.ic_time > 65'000) // 16bit nanoseconds
    delayMicroseconds(run.config.ic_time / 1000);
  else
    delayNanoseconds(run.config.ic_time);
  mode::RealManualControl::to_op();
  elapsedMicros actual_op_time_timer;

  if(buffer) {
    for(uint32_t sample=0; sample < num_samples; sample++) {
      // Assumption: Passing the next line requires the amount of time "sampling_time_us"
      daq.sample_raw(buffer + sample*num_channels);
      if(sampling_sleep_us)
        delayMicroseconds(sampling_sleep_us);
    }
    if(optime_left_us)
      delayMicroseconds(optime_left_us);
  } else {
    if(run.config.op_time > 100'000'000) // 100ms
      delay(run.config.op_time / 1'000'000); // millisecond resolution sleep
    else if(run.config.op_time > 65'000) // 16bit nanoseconds
      delayMicroseconds(run.config.op_time / 1000);
    else
      delayNanoseconds(run.config.op_time);
  }

  uint32_t actual_op_time_us = actual_op_time_timer;
  mode::RealManualControl::to_halt();
  
  if(buffer) {
    alt_run_data_handler->handle(buffer, num_samples, num_channels, run);
    free(buffer);
  }

  auto res = (num_channels && !buffer) ? RunState::ERROR : RunState::DONE;
  auto result = run.to(res, actual_op_time_us*1000);
  if(run.config.write_run_state_changes)
    state_change_handler->handle(result, run);
}

// NOT FLASHMEM
void run::RunManager::run_next_flexio(run::Run &run, RunStateChangeHandler *state_change_handler, RunDataHandler *run_data_handler) {
  run_data_handler->prepare(run);
  bool daq_error = false;

  daq::FlexIODAQ daq_{run, run.daq_config, run_data_handler};
  daq_.reset();
  mode::FlexIOControl::reset();
  if (!mode::FlexIOControl::init(run.config.ic_time, run.config.op_time,
                                 run.config.halt_on_overload ? mode::OnOverload::HALT
                                                             : mode::OnOverload::IGNORE,
                                 mode::OnExtHalt::IGNORE) or
      !daq_.init(0)) {
    LOG_ERROR("Error while initializing state machine or daq for run.")
    auto change = run.to(RunState::ERROR, 0);
    state_change_handler->handle(change, run);
    return;
  }

  run_data_handler->init();
  daq_.enable();
  mode::FlexIOControl::force_start();
  delayMicroseconds(1);

  while (!mode::FlexIOControl::is_done()) {
    if (!daq_.stream()) {
      LOG_ERROR("Streaming error, most likely data overflow.");
      daq_error = true;
      break;
    }
  }
  mode::FlexIOControl::to_end();

  // When a data sample must be gathered very close to the end of OP duration,
  // it takes about 1 microseconds for it to end up in the DMA buffer.
  // This is hard to check for, since the DMA active flag is only set once the DMA
  // is triggered by the last CLK pulse, which is after around 1 microsecond.
  // Easiest solution is to wait for it.
  delayMicroseconds(1);
  // Stream out remaining partially filled buffer
  if (!daq_.stream(true)) {
    LOG_ERROR("Streaming error during final partial stream.");
    daq_error = true;
  }

  auto actual_op_time = mode::FlexIOControl::get_actual_op_time();

  // Finalize data acquisition
  if (!daq_.finalize()) {
    LOG_ERROR("Error while finalizing data acquisition.")
    daq_error = true;
  }

  if (daq_error) {
    auto change = run.to(RunState::ERROR, actual_op_time);
    state_change_handler->handle(change, run);
    return;
  }

  // DONE
  auto change = run.to(RunState::DONE, actual_op_time);
  state_change_handler->handle(change, run);
}

int run::RunManager::start_run(JsonObjectConst msg_in, JsonObject &msg_out) {
  if (!msg_in.containsKey("id") or !msg_in["id"].is<std::string>())
    return 1;

  if(msg_in.containsKey("end_repetitive") && msg_in["end_repetitive"].as<bool>()) {
    end_repetitive_runs();
  }

  if(msg_in.containsKey("clear_queue") && msg_in["clear_queue"].as<bool>()) {
    clear_queue();
  }

  // Create run and put it into queue
  auto run = run::Run::from_json(msg_in);
  queue.push(std::move(run));
  return 0 /* success */;
}
