// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <map>
#include <string>

#include "handler.h"
#include "user/auth.h"


namespace msg {

namespace handlers {

/**
 * The Message Registry holds the list of all known message types.
 * 
 * The DynamicRegistry implementation uses an heap-allocated std::map as well as
 * Handlers which can be null pointers.
 * 
 * \ingroup Singletons
 **/
class DynamicRegistry {
  struct RegistryEntry { // "named tuple"
    MessageHandler* handler;
    user::auth::SecurityLevel clearance;
    int result_code_prefix; // Some decimal error code prefix, inspired by errno, HRESULT, etc.
  };

  std::map<std::string, RegistryEntry> entries;

  int result_code_counter = 1, result_code_increment = 100;

public:
  MessageHandler *get(const std::string& msg_type);
  user::auth::SecurityLevel requiredClearance(const std::string& msg_type);

  bool set(const std::string& msg_type, MessageHandler *handler, user::auth::SecurityLevel minimumClearance);
  bool set(const std::string& msg_type, int result_code_prefix, MessageHandler *handler, user::auth::SecurityLevel minimumClearance);

  void dump(); //< for debugging: Print Registry configuration to Serial
  void write_handler_names_to(JsonArray& target); ///< for structured output

  void init(); ///< Actual registration of all handlers in code.
};

// the singleton instance
extern DynamicRegistry Registry;


} // namespace handlers

// TODO: Look for a better place for this functions

/**
  * Handles a message, according to the registry.
  * Passes return codes as they appear from the handlers.
  * 
  * \todo Could be a method of the Message Registry
  **/
int handleMessage(JsonObjectConst envelope_in, JsonObject& envelope_out, user::auth::AuthentificationContext &user_context);

void process_serial_input();

} // namespace msg