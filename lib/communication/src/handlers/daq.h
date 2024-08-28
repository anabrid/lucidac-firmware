// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "daq/daq.h"
#include "protocol/handler.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class OneshotDAQHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(daq::OneshotDAQ().sample(msg_in, msg_out));
  }
};

} // namespace handlers
} // namespace msg
