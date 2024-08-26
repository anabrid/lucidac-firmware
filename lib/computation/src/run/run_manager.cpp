// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "run/run_manager.h"

#include <cstdlib>
#include <cmath>
#include <Arduino.h>

#include "daq/daq.h"
#include "utils/logging.h"

run::RunManager run::RunManager::_instance{};

void run::RunManager::run_next(RunStateChangeHandler *state_change_handler, RunDataHandler *run_data_handler) {
  // TODO: Improve handling of queue, especially the queue.pop() later.
  auto run = queue.front();
  if(run.config.no_streaming)
    run_next_traditional(run, state_change_handler, run_data_handler);
  else
    run_next_flexio(run, state_change_handler, run_data_handler);
  queue.pop();
}

void run::RunManager::run_next_traditional(run::Run &run, RunStateChangeHandler *state_change_handler, RunDataHandler *run_data_handler) {
  run_data_handler->prepare(run);

  daq::OneshotDAQ daq;
  daq.init(0);

  // measure how long an single ADC sampling takes with the current implementation.
  // This is right now around 15us.
  elapsedMicros sampling_time_us_counter; // class from teensy elpasedMillis.h
  daq.sample_raw();
  uint32_t sampling_time_us = sampling_time_us_counter;
  // TODO: There is a slight dither around 15us -- 16us, could do the averaging call to
  //       improve the statistics.

  // here, one sample is one data aquisition which always yields all channels.
  constexpr uint32_t max_num_samples = 16'384;
  const int num_channels = run.daq_config.get_num_channels();
  const uint32_t optime_us = run.config.op_time / 1000;

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
  LOGMEV("optime_us=%d, sampling_time_us=%d, sampling_sleep_us=%d, num_samples=%d, num_channels=%d", optime_us, sampling_time_us, sampling_sleep_us, num_samples, num_channels);

  // we use malloc because new[] raises an exception if allocation fails but we cannot catch it with -fno-exception.
  // we don't use the stack because we expect the heap to have more memory left...
  auto buffer = (uint16_t*) malloc(num_buffer_entries * sizeof(uint16_t));
  if(!buffer) {
    LOG_ERROR("Could not allocated a large run buffer for a traditional Run.");
  }

  mode::RealManualControl::enable();

  mode::RealManualControl::to_ic();
  delayNanoseconds(run.config.ic_time);
  mode::RealManualControl::to_op();
  elapsedMicros actual_op_timer;
  if(buffer) {
    for(uint32_t sample=0; sample < num_samples; sample++) {
      // Assumption: The next line requires the time sampling_time_us to execute
      daq.sample_raw(buffer + sample*num_channels);
      if(sampling_sleep_us)
        delayMicroseconds(sampling_sleep_us);
    }
    if(optime_left_us)
      delayMicroseconds(optime_left_us);
  } else {
    delayMicroseconds(optime_us);
  }
  mode::RealManualControl::to_halt();
  uint32_t actual_op_time_us = actual_op_timer;
  
  run::RunStateChange result;
  if(buffer) {
    // Data transfer happens after the run finished.
    run_data_handler->init();

    // We abuse the existing infrastructure which can send buffer chunks of size
    // daq::dma::BUFFER_SIZE but no more. Unfortunately, also this buffer has to be uint32_t,
    // probably for DMA reasons at aquire time. So we have to call this repeatedly to have it
    // write out our little buffer to the client, which we also have to convert to uint32_t.

    // num_buffer_entries == num_chunks * writer_bufsize + last_outer_count
    //                    == num_chunks * ()

    size_t writer_bufsize    = daq::dma::BUFFER_SIZE / 2;
    size_t num_chunks        = num_buffer_entries / writer_bufsize;
    size_t last_chunk        = num_buffer_entries % writer_bufsize;
    size_t outer_count       = writer_bufsize     / num_channels;
    size_t last_outer_count  = last_chunk         / num_channels;

    LOGMEV("Writing out num_chunks=%d, last_chunk=%d, outer_count=%d last_outer_count=%d", num_chunks, last_chunk, outer_count, last_outer_count);

    CrashReport.breadcrumb(1, 1111111);
    auto converted = (uint32_t*)malloc(writer_bufsize * sizeof(uint32_t));
    if(!converted) {
      LOG_ERROR("Could not allocated writeout buffer");
      result = run.to(RunState::ERROR, 0);  
    } else {
      // full chunks to write out (with length outer_count each)
      if(num_chunks) {
        for(size_t chunk=0; chunk < num_chunks; chunk++) {
          CrashReport.breadcrumb(2, chunk);
          for(size_t i=0; i < writer_bufsize; i++) {
            CrashReport.breadcrumb(3, i);
            converted[i] = buffer[chunk*outer_count*num_channels + i];
          }
          CrashReport.breadcrumb(1, 22222222);
          run_data_handler->handle(converted, outer_count, num_channels, run);
          CrashReport.breadcrumb(1, 33333333);
        }
      }
      // last partial chunk
      if(last_chunk) {
        for(size_t i=0; i < last_outer_count; i++) {
          CrashReport.breadcrumb(3, i);
          converted[i] = buffer[num_chunks*outer_count*num_channels + i];
        }
        CrashReport.breadcrumb(1, 444444444);
        run_data_handler->handle(converted, last_outer_count, num_channels, run);
        CrashReport.breadcrumb(1, 555555555);
      }
      free(converted);
      result = run.to(RunState::DONE, actual_op_time_us*1000);
    }
    free(buffer);
  } else {
    result = run.to(RunState::ERROR, 0);
  }
  state_change_handler->handle(result, run);
}

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
  // Create run and put it into queue
  auto run = run::Run::from_json(msg_in);
  queue.push(std::move(run));
  return 0 /* success */;
}
