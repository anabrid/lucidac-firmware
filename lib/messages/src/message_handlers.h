// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <map>

#include "user_auth.h"


namespace msg {

namespace handlers {

class MessageHandler {
public:
  /**
   * @return Whether the handling was successful or not 
   **/
  virtual bool handle(JsonObjectConst msg_in, JsonObject &msg_out) = 0;
};

class Registry {
  static std::map<std::string, MessageHandler *> _registry;
  static std::map<std::string, auth::SecurityLevel> _clearance;

public:
  static MessageHandler *get(const std::string& msg_type);
  static auth::SecurityLevel requiredClearance(const std::string& msg_type);
  static bool set(const std::string& msg_type, msg::handlers::MessageHandler *handler, auth::SecurityLevel minimumClearance);

  static void dump(); // for debugging: Print Registry configuration to Serial
  static void write_handler_names_to(JsonArray& target);
};

class PingRequestHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class GetSystemStatus : public MessageHandler {
  auth::UserPasswordAuthentification& _auth;
public:
  GetSystemStatus(auth::UserPasswordAuthentification& auth) : _auth(auth) {}
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class HelpHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers

} // namespace msg