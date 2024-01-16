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
  if (!mode::FlexIOControl::init(run.config.ic_time, run.config.op_time) or !daq_.init(0)) {
    LOG_ERROR("Error while initializing state machine or daq for run.")
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

bool msg::handlers::StartRunRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  if (!msg_in.containsKey("id") or !msg_in["id"].is<std::string>())
    return false;
  // Create run and put it into queue
  auto run = run::Run::from_json(msg_in);
  manager.queue.push(std::move(run));
  return true;
}
