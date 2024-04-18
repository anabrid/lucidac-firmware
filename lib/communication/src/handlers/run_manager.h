// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "protocol/handler.h"
#include "run/run_manager.h"

namespace msg {

namespace handlers {

class StartRunRequestHandler : public MessageHandler {
protected:
  run::RunManager &manager;

public:
  explicit StartRunRequestHandler(run::RunManager &run_manager) : manager(run_manager) {}

  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    if (!msg_in.containsKey("id") or !msg_in["id"].is<std::string>())
      return false;
    // Create run and put it into queue
    auto run = run::Run::from_json(msg_in);
    manager.queue.push(std::move(run));
    return true;
  };
};

} // namespace handlers

} // namespace msg
