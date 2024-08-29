// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "protocol/handler.h"
#include "run/run_manager.h"

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class StartRunRequestHandler : public MessageHandler {
protected:
  run::RunManager &manager;

public:
  explicit StartRunRequestHandler(run::RunManager &run_manager) : manager(run_manager) {}

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(manager.start_run(msg_in, msg_out));
  }
};

} // namespace handlers

} // namespace msg
