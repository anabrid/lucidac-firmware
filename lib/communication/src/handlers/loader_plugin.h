// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "plugin/plugin.h"
#include "protocol/handler.h"

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class LoadPluginHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(loader::PluginLoader.load_and_execute(msg_in, msg_out));
  }
};

/// @ingroup MessageHandlers
class UnloadPluginHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(loader::PluginLoader.unload(msg_in, msg_out));
  }
};

} // namespace handlers

} // namespace msg
