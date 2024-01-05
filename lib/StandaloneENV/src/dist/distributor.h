// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

//#include "message_handlers.h"
#include "ArduinoJson.h"

/**
 * The distributor headerfile collects constants as C preprocessor headers.
 * They are composed at compile-time by a python script.
 * 
 * In the application context, some of these variables are considered secret (such as the
 * default admin password).
 * 
 * Since the preprocessor directives are not namespaced or typed (they are all C strings by
 * definition), some helpers functions are here as functions.
 * 
 **/
namespace dist {

  // Get a short oneline identifier name
  const char* ident();

  // Get the program memory database as already-serialized JSON String.
  const char* as_json(bool include_secrets=true);

  // Write the distributor database into an existing JSON document
  void write_to_json(JsonObject target, bool include_secrets=true);

} // namespace dist

#include "distributor_generated.h"

namespace msg {

namespace handlers {

// see messages/src/message_handlers.h for the msg::handlers::GetSystemStatus::handle
// which also includes the distributor information

/*
class GetDistributorHandler : public SettingsHandler {
public:
  using SettingsHandler::SettingsHandler;
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

*/

} // namespace handlers

} // namespace msg 
