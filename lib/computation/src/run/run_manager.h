// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "run/run.h"

namespace client {
  class StreamingRunDataNotificationHandler;
} // namespace client

namespace carrier {
  class Carrier;
}

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

  /// Returns true on success
  bool end_repetitive_runs() {
    if (!queue.empty()) {
      queue.front().config.repetitive = false;
      return true;
    }
    return false;
  }

  /// Clears the run queue
  void clear_queue() {
    queue = {};
    // if this does not work, try while(!Q.empty()) Q.pop();
  }

  void run_next(carrier::Carrier &carrier_, run::RunStateChangeHandler *state_change_handler,
                run::RunDataHandler *run_data_handler,
                client::StreamingRunDataNotificationHandler *alt_run_data_handler);

  void run_next_flexio(run::Run &run, run::RunStateChangeHandler *state_change_handler, run::RunDataHandler *run_data_handler);
  void run_next_traditional(run::Run &run, run::RunStateChangeHandler *state_change_handler, run::RunDataHandler *run_data_handler, client::StreamingRunDataNotificationHandler *alt_run_data_handler);

  ///@ingroup User-Functions
  int start_run(JsonObjectConst msg_in, JsonObject &msg_out);
};

} // namespace run

