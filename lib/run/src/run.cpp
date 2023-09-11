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

#include "mode.h"
#include "run.h"

#include <utility>

run::RunManager run::RunManager::_instance{};

void run::RunManager::run_next(run::RunStateChangeHandler *state_change_handler) {
  auto run = queue.front();

  mode::FlexIOControl::init(run.config.ic_time, run.config.op_time);
  mode::FlexIOControl::force_start();
  /*
  // Take off
  auto change = run.to(RunState::TAKE_OFF, 0);
  state_change_handler->handle(change, run);
  // IC
  change = run.to(RunState::IC, 0);
  state_change_handler->handle(change, run);
  delayNanoseconds(run.config.ic_time);
  // OP
  change = run.to(RunState::OP, run.config.ic_time);
  state_change_handler->handle(change, run);
  delayNanoseconds(run.config.op_time);
  // OP end
  change = run.to(RunState::OP_END, run.config.ic_time+run.config.op_time);
  state_change_handler->handle(change, run);
  // DONE
  change = run.to(RunState::DONE, run.config.ic_time+run.config.op_time);
  state_change_handler->handle(change, run);
   */

  queue.pop();
}

run::RunConfig run::RunConfig::from_json(JsonObjectConst &json) {
  return {
      .ic_time = json["ic_time"], .op_time = json["op_time"], .halt_on_overload = json["halt_on_overload"]};
}

run::Run::Run(std::string id, const run::RunConfig &config) : id(std::move(id)), config(config) {}

run::Run run::Run::from_json(JsonObjectConst &json) {
  auto json_run_config = json["config"].as<JsonObjectConst>();
  auto run_config = RunConfig::from_json(json_run_config);
  return {json["id"].as<std::string>(), run_config};
}

run::RunStateChange run::Run::to(run::RunState new_state, unsigned int t) {
  auto old = state;
  state = new_state;
  return {t, old, state};
}

bool msg::handlers::StartRunRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  if (!msg_in.containsKey("id") or !msg_in["id"].is<std::string>())
    return false;
  // Create run and put it into queue
  auto run = run::Run::from_json(msg_in);
  manager.queue.push(std::move(run));
  return true;
}
