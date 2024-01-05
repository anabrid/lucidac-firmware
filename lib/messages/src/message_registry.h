// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <map>

#include "message_handlers.h"
#include "user_auth.h"


namespace msg {

namespace handlers {

/**
 * The Message Registry holds the list of all known message types.
 * 
 * The DynamicRegistry implementation uses an heap-allocated std::map as well as
 * Handlers which can be null pointers.
 **/
class DynamicRegistry {
  struct RegistryEntry { // "named tuple"
    MessageHandler* handler;
    user::auth::SecurityLevel clearance;
  };

  std::map<std::string, RegistryEntry> entries;

public:
  MessageHandler *get(const std::string& msg_type);
  user::auth::SecurityLevel requiredClearance(const std::string& msg_type);
  bool set(const std::string& msg_type, MessageHandler *handler, user::auth::SecurityLevel minimumClearance);

  void dump(); //< for debugging: Print Registry configuration to Serial
  void write_handler_names_to(JsonArray& target); ///< for structured output

  void init(); ///< Actual registration of all handlers in code.
};

// the singleton instance
extern DynamicRegistry Registry;

class PingRequestHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class GetSystemStatus : public MessageHandler {
  user::auth::UserPasswordAuthentification& _auth;
public:
  GetSystemStatus(user::auth::UserPasswordAuthentification& auth) : _auth(auth) {}
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class HelpHandler : public MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers

} // namespace msg