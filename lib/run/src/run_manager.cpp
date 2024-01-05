// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "run_manager.h"

#include "daq.h"
#include "logging.h"

run::RunManager run::RunManager::_instance{};

void run::RunManager::run_next(RunStateChangeHandler *state_change_handler, RunDataHandler *run_data_handler) {
  // TODO: Improve handling of queue, especially the queue.pop() later.
  auto run = queue.front();
  run_data_handler->prepare(run);

  daq::FlexIODAQ daq_{run, run.daq_config, run_data_handler};
  daq_.reset();
  if (!mode::FlexIOControl::init(run.config.ic_time, run.config.op_time)) {
    LOG_ERROR("Error while initializing state machine.")
    auto change = run.to(RunState::ERROR, 0);
    state_change_handler->handle(change, run);
    queue.pop();
    return;
  }
  if(!daq_.init(0)) {
    LOG_ERROR("Error while initializing daq for run.")
    auto change = run.to(RunState::ERROR, 0);
    state_change_handler->handle(change, run);
    queue.pop();
    return;
  }

  run_data_handler->init();
  daq_.enable();
  Serial.println("Run starts.");
  mode::FlexIOControl::force_start();
  delayMicroseconds(1);

  while (!mode::FlexIOControl::is_done()) {
    if(!daq_.stream()) {
      LOG_ERROR("Streaming error, most likely data overflow.");
      break ;
    }
  }
  mode::FlexIOControl::to_end();

  // Sometimes, DMA was not yet done, e.g. op_time = 6000.
  // TODO: Handle this better by checking for DMA completeness.
  delayMicroseconds(5);
  //if(!daq_.stream()) {
  //  LOG_ERROR("Streaming error, most likely data overflow.");
  //}

  auto actual_op_time = mode::FlexIOControl::get_actual_op_time();

  // Finalize data acquisition
  if (!daq_.finalize()) {
    LOG_ERROR("Error while finalizing data acquisition.")
    auto change = run.to(RunState::ERROR, actual_op_time);
    state_change_handler->handle(change, run);
    queue.pop();
    return;
  }

  // DONE
  auto change = run.to(RunState::DONE, actual_op_time);
  state_change_handler->handle(change, run);
  queue.pop();
}

int msg::handlers::StartRunRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  if (!msg_in.containsKey("id") or !msg_in["id"].is<std::string>())
    return 1;
  // Create run and put it into queue
  auto run = run::Run::from_json(msg_in);
  manager.queue.push(std::move(run));
  return success;
}
