// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

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