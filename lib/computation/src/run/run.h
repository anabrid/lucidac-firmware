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

#pragma once

#include <ArduinoJson.h>
#include <queue>
#include <string>

#include "daq_base.h"
#include "message_handlers.h"
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

class RunConfig {
public:
  unsigned int ic_time = 100'000;
  unsigned long long op_time = 500'000'000;
  bool halt_on_overload = true;

  static RunConfig from_json(JsonObjectConst &json);
};

class Run {
public:
  const std::string id;
  RunConfig config;
  RunState state = RunState::NEW;
  daq::DAQConfig daq_config;

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