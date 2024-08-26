// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "run/run.h"

namespace client {
  class StreamingRunDataNotificationHandler;
} // namespace client

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

  void run_next(
    run::RunStateChangeHandler *state_change_handler,
    run::RunDataHandler *run_data_handler,
    client::StreamingRunDataNotificationHandler *alt_run_data_handler);

  void run_next_flexio(run::Run &run, run::RunStateChangeHandler *state_change_handler, run::RunDataHandler *run_data_handler);
  void run_next_traditional(run::Run &run, run::RunStateChangeHandler *state_change_handler, run::RunDataHandler *run_data_handler, client::StreamingRunDataNotificationHandler *alt_run_data_handler);

  ///@ingroup User-Functions
  int start_run(JsonObjectConst msg_in, JsonObject &msg_out);
};

} // namespace run

