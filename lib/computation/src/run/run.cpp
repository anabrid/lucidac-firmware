// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "run/run.h"

#include "utils/logging.h"
#include <utility>

FLASHMEM run::RunConfig run::RunConfig::from_json(JsonObjectConst &json) {
  // ATTENTION: ArduinoJSON cannot easily handle 64bit integers,
  //        cf. https://arduinojson.org/v6/api/config/use_long_long/
  // Therefore, client libraries should make use of appropriate units
  // which are summed here together.

  uint64_t
    ic_time_ms  = json["ic_time_ms"],
    ic_time_us  = json["ic_time_us"],
    ic_time_ns  = json["ic_time_ns"],
    ic_time_def = json["ic_time"], // Nanoseconds per default

    op_time_ms  = json["op_time_ms"],
    op_time_us  = json["op_time_us"],
    op_time_ns  = json["op_time_ns"],
    op_time_def = json["op_time"]; // Nanoseconds per default

  uint64_t us = 1000, ms = 1'000'000;

  run::RunConfig run;

  run.ic_time = ic_time_def + ic_time_ns + us*ic_time_us + ms*ic_time_ms;
  run.op_time = op_time_def + op_time_ns + us*op_time_us + ms*op_time_ms;

  //LOG_MEV("ic_time_ns = %lld, op_time_ns=%lld", run.ic_time, run.op_time);

  // default values for the following keys are given
  // in the RunConfig class definition.

  if(json.containsKey("halt_on_overload"))
    run.halt_on_overload = json["halt_on_overload"];

  if(json.containsKey("streaming"))
    run.streaming = json["streaming"];

  if(json.containsKey("repetitive"))
    run.repetitive = json["repetitive"];

  if(json.containsKey("write_run_state_changes"))
    run.write_run_state_changes = json["write_run_state_changes"];

  if(json.containsKey("calibrate"))
    run.calibrate = json["calibrate"];

  return run;
}

FLASHMEM
run::Run::Run(std::string id, const run::RunConfig &config)
    : id(std::move(id)), config(config), daq_config{} {}

FLASHMEM
run::Run::Run(std::string id, const run::RunConfig &config, const daq::DAQConfig &daq_config)
    : id(std::move(id)), config(config), daq_config(daq_config) {}

FLASHMEM
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

FLASHMEM
run::RunStateChange run::Run::to(run::RunState new_state, unsigned int t) {
  auto old = state;
  state = new_state;
  return {t, old, state};
}
