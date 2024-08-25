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

  constexpr int num_samples = 16'384;
  const int num_channels = run.daq_config.get_num_channels();
  const int num_buffer_entries = num_samples * num_channels;
  // we use malloc because new[] raises an exception if allocation fails but we cannot catch it with -fno-exception.
  // we don't use the stack because we expect the heap to have more memory left...
  auto buffer = (uint16_t*) malloc(num_buffer_entries * sizeof(uint16_t));
  if(!buffer) {
    LOG_ERROR("Could not allocated a large run buffer for a traditional Run.");
    //auto change = run.to(RunState::ERROR, 0);
    //state_change_handler->handle(change, run);
  }
  // we ignore run.daq_config.get_sample_rate() because it is much simpler to just always fill up the buffer
  uint32_t dt_sample_ns = /*math.ceil*/ run.config.op_time / num_buffer_entries;

  mode::RealManualControl::enable();

  mode::RealManualControl::to_ic();
  delayNanoseconds(run.config.ic_time);
  mode::RealManualControl::to_op();
  if(buffer) {
    uint32_t pos=0;
    for(uint32_t samples=0; samples < num_samples; samples++) {
      // need another local buffer solely because the global one could be too small at the last sample.
      // this most likely is still faster then buffer[pos++] = daq.sample_raw(channel);
      auto sample_buf = daq.sample_raw();
      for(uint8_t channel=0; channel < num_channels; channel++)
        buffer[pos++] = sample_buf[channel];
      delayNanoseconds(dt_sample_ns);
    }
  } else {
    delayNanoseconds(run.config.op_time);
  }
  mode::RealManualControl::to_halt();
  
  run::RunStateChange result;
  if(buffer) {
    // Data transfer happens after the run finished.
    run_data_handler->init();

    // We abuse the existing infrastructure which can send buffer chunks of size
    // daq::dma::BUFFER_SIZE but no more. Unfortunately, also this buffer has to be uint32_t,
    // probably for DMA reasons at aquire time. So we have to call this repeatedly to have it
    // write out our little buffer to the client, which we also have to convert to uint32_t.

    auto num_chunk_entries = daq::dma::BUFFER_SIZE / sizeof(uint16_t);
    size_t outer_count = /*floor*/ num_chunk_entries / num_channels;
    size_t num_chunks = ceil((float)num_buffer_entries / num_chunk_entries);
    size_t writeout_buffer_size = sizeof(uint32_t)*outer_count*num_channels;
    LOGMEV("Writing out num_chunks=%d, outer_count=%d", num_chunks, outer_count);

    uint32_t converted[writeout_buffer_size] = {0};
    for(size_t chunk=0; chunk < num_chunks; chunk++) {
      for(size_t i=0; i < outer_count*num_channels; i++)
        converted[i] = buffer[chunk*outer_count*num_channels + i];
      run_data_handler->handle(converted, outer_count, num_channels, run);
    }

    free(buffer);
    result = run.to(RunState::DONE /*, actual_op_time*/);
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
