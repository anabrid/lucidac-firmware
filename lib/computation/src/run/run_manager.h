// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "run/run.h"

namespace run {

class RunManager {
private:
  static RunManager _instance;

protected:
  RunManager() = default;

public:
  std::queue<run::Run> queue;

  RunManager(RunManager &other) = delete;
  void operator=(const RunManager &other) = delete;

  static RunManager &get() { return _instance; }

  void run_next(run::RunStateChangeHandler *state_change_handler, run::RunDataHandler *run_data_handler);

  // exposed end-user function
  int start_run(JsonObjectConst msg_in, JsonObject &msg_out);
};

} // namespace run

