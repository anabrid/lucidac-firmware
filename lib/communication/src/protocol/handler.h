// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <map>

namespace msg {

namespace handlers {

class MessageHandler {
public:
  virtual bool handle(JsonObjectConst msg_in, JsonObject &msg_out) = 0;
};

class Registry {
  static std::map<std::string, MessageHandler *> _registry;

public:
  static MessageHandler *get(const std::string& msg_type);
  static bool set(const std::string& msg_type, msg::handlers::MessageHandler *handler, bool overwrite = false);
};

class PingRequestHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers

} // namespace msg