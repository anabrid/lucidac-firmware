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
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(run::RunManager::get().start_run(msg_in, msg_out));
  }
};

class StopRunRequestHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    auto success = run::RunManager::get().end_repetitive_runs();
    return error(success ? 0 : 1);
  }
};

} // namespace handlers

} // namespace msg
