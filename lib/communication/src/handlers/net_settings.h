// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "net/auth.h"
#include "net/ethernet.h"
#include "nvmconfig/persistent.h"
#include "protocol/handler.h"

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class GetNetworkSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    nvmconfig::PersistentSettingsWriter::get().toJson(msg_out, nvmconfig::Context::User);
    return success;
  }
};

/// @ingroup MessageHandlers
class SetNetworkSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    nvmconfig::PersistentSettingsWriter::get().fromJson(msg_in, nvmconfig::Context::User);

    if (!msg_in.containsKey("no_write"))
      nvmconfig::PersistentSettingsWriter::get().write_to_eeprom();

    // Since ArduinoJSON has no good methods of finding whether data
    // deserialization worked correctly, return the new values to the
    // client so he can check by himself.
    GetNetworkSettingsHandler().handle(/*is ignored*/ msg_in, msg_out);

    return success;
  }
};

/// @ingroup MessageHandlers
class NetworkStatusHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    net::status(msg_out);
    return success;
  }
};

/// @ingroup MessageHandlers
class ResetNetworkSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    // TODO: Must also reset all other config classes!
    //       Probably better reset all nvmconfig info or so.
    net::StartupConfig::get().reset_defaults();
    return success;
  }
};

} // namespace handlers

} // namespace msg
