// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <queue>
#include <string>

#include "daq/base.h"
#include "mode/mode.h"

namespace run {

enum class RunState { NEW, ERROR, DONE, QUEUED, TAKE_OFF, IC, OP, OP_END, TMP_HALT, _COUNT __attribute__((unused))
};

constexpr const char* RunStateNames[static_cast<unsigned int>(RunState::_COUNT)] = {
    "NEW", "ERROR", "DONE", "QUEUED", "TAKE_OFF", "IC", "OP", "OP_END", "TMP_HALT"
};

class RunStateChange {
public:
  unsigned long long t;
  RunState old;
  RunState new_;
};

/**
 * The RunConfig data structure defines the properties for a run. It is specified
 * by the user and forms the properties of a @see Run.
 **/
class RunConfig {
public:
  unsigned int ic_time = 100'000;            ///< Requested Initial Conditions time in Nanoseconds
  unsigned long long op_time = 500'000'000;  ///< Requested Operations time in Nanoseconds
  bool halt_on_overload = true;              ///< Whether to halt the computation on an overload condition
  bool no_streaming = false;                 ///< Request traditional inexact non-streaming run (Non-FlexIO/DMA)

  static RunConfig from_json(JsonObjectConst &json);
};

class Run {
public:
  const std::string id; ///< (User-provided) Supposed to be a UUID which allows to identify Out-of-band messages
  RunConfig config;     ///< (User-provided) timing requests
  RunState state = RunState::NEW; ///< (System-steered)
  daq::DAQConfig daq_config;  ///< (User-provided) Data Aquisition request

protected:
  std::queue<RunStateChange, std::array<RunStateChange, 7>> history;

public:
  Run(std::string id, const RunConfig &config, const daq::DAQConfig &daq_config);
  Run(std::string id, const RunConfig &config);
  static Run from_json(JsonObjectConst &json);

  RunStateChange to(RunState new_state, unsigned int t = 0);
};

class RunStateChangeHandler {
public:
  virtual void handle(run::RunStateChange change, const run::Run &run) = 0;
};

class RunDataHandler {
public:
  volatile bool first_data = false;
  volatile bool last_data = false;
  virtual void init() {};
  virtual void handle(volatile uint32_t *data, size_t outer_count, size_t inner_count, const run::Run &run) = 0;
  virtual void stream(volatile uint32_t *buffer, run::Run &run) = 0;
  virtual void prepare(Run& run) = 0;
};

} // namespace run