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

#include "run.h"

#include "client.h"
#include "daq.h"
#include "logging.h"
#include "mode.h"

#include <bitset>
#include <utility>

run::RunManager run::RunManager::_instance{};

void run::RunManager::run_next(RunStateChangeHandler *state_change_handler, RunDataHandler *run_data_handler) {
  auto run = queue.front();

  // LED used as debug
  // TODO: REMOVE ALL DEBUG
  pinMode(LED_BUILTIN, OUTPUT);
  daq::FlexIODAQ daq_{};
  daq_.reset();
  if (!mode::FlexIOControl::init(run.config.ic_time, run.config.op_time) or
      !daq_.init(daq::DEFAULT_SAMPLE_RATE)) {
    LOG_ERROR("Error while initializing state machine or daq for run.")
    auto change = run.to(RunState::ERROR, 0);
    state_change_handler->handle(change, run);
    queue.pop();
    return;
  }
  daq_.enable();
  mode::FlexIOControl::force_start();
  mode::FlexIOControl::delay_till_done();
  // Sometimes, DMA was not yet done, e.g. op_time = 6000.
  // TODO: Handle this better by checking for DMA completeness.
  delayMicroseconds(5);
  auto actual_op_time = mode::FlexIOControl::get_actual_op_time();

  // TODO: REMOVE ALL DEBUG
  Serial.println("Error flags");
  Serial.println(std::bitset<32>(daq_.flexio->port().SHIFTERR).to_string().c_str());
  Serial.println("DMA channel properties");
  Serial.println(daq_.get_dma_channel().TCD->CITER);
  Serial.println("DMA buffer content");
  for (auto data : decltype(daq_)::dma_buffer) {
    Serial.println(std::bitset<32>(data).to_string().c_str());
  }
  Serial.println("Normalized buffer content");
  for (auto data : daq::get_normalized_buffer()) {
    Serial.println(data);
  }
  Serial.println("Float buffer content");
  for (auto data : daq::get_float_buffer()) {
    Serial.println(data);
  }
  digitalToggleFast(LED_BUILTIN);
  run_data_handler->handle(daq::get_float_buffer().data(), daq::BUFFER_SIZE/daq::NUM_CHANNELS, daq::NUM_CHANNELS, run);
  digitalToggleFast(LED_BUILTIN);

  // DONE
  auto change = run.to(RunState::DONE, actual_op_time);
  state_change_handler->handle(change, run);
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
