// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "run/run.h"

#include <utility>

run::RunConfig run::RunConfig::from_json(JsonObjectConst &json) {
  return {
      .ic_time = json["ic_time"],
      .op_time = json["op_time"],
      .halt_on_overload = json["halt_on_overload"],
      .no_streaming = json["no_streaming"],
      .repetitive = json["repetitive"]
    };
}

run::Run::Run(std::string id, const run::RunConfig &config)
    : id(std::move(id)), config(config), daq_config{} {}

run::Run::Run(std::string id, const run::RunConfig &config, const daq::DAQConfig &daq_config)
    : id(std::move(id)), config(config), daq_config(daq_config) {}

run::Run run::Run::from_json(JsonObjectConst &json) {
  auto json_run_config = json["config"].as<JsonObjectConst>();
  auto run_config = RunConfig::from_json(json_run_config);
  auto daq_config = daq::DAQConfig::from_json(json["daq_config"].as<JsonObjectConst>());
  auto id = json["id"].as<std::string>();
  if (id.size() != 32 + 4) {
    id = "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx";
  }
  return {id, run_config, daq_config};
}

run::RunStateChange run::Run::to(run::RunState new_state, unsigned int t) {
  auto old = state;
  state = new_state;
  return {t, old, state};
}
