// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "run/run.h"

#include <utility>

run::RunConfig run::RunConfig::from_json(JsonObjectConst &json) {
  // ATTENTION: ArduinoJSON cannot easily handle 64bit integers,
  //        cf. https://arduinojson.org/v6/api/config/use_long_long/
  // Therefore, client libraries should make use of appropriate units
  // which are summed here together.

  uint32_t
    ic_time_ms  = json["ic_time_ms"],
    ic_time_us  = json["ic_time_us"],
    ic_time_ns  = json["ic_time_ns"],
    ic_time_def = json["ic_time"], // Nanoseconds per default

    op_time_ms  = json["op_time_ms"],
    op_time_us  = json["op_time_us"],
    op_time_ns  = json["op_time_ns"],
    op_time_def = json["op_time"]; // Nanoseconds per default

  uint64_t us = 1000, ms = 1'000'000;

  return {
      .ic_time = ic_time_def + ic_time_ns + us*ic_time_us + ms*ic_time_ms,
      .op_time = op_time_def + op_time_ns + us*op_time_us + ms*op_time_ms,
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
