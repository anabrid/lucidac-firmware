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

#include "message_handlers.h"

#include "persistent_eth.h"
#include "user_auth.h"
#include "distributor.h"

std::map<std::string, msg::handlers::MessageHandler *> msg::handlers::Registry::_registry{
    {"ping", new PingRequestHandler{}}};

std::map<std::string, auth::SecurityLevel> msg::handlers::Registry::_clearance{
    {"ping", auth::SecurityLevel::RequiresNothing}};

msg::handlers::MessageHandler *msg::handlers::Registry::get(const std::string &msg_type) {
  auto found = _registry.find(msg_type);
  if (found != _registry.end()) {
    return found->second;
  } else {
    return nullptr;
  }
}

auth::SecurityLevel msg::handlers::Registry::requiredClearance(const std::string &msg_type) {
  auto found = _clearance.find(msg_type);
  if (found != _clearance.end()) {
    return found->second;
  } else {
    return auth::SecurityLevel::RequiresNothing;
  }
}

bool msg::handlers::Registry::set(const std::string &msg_type, msg::handlers::MessageHandler *handler,
                                  auth::SecurityLevel minimumClearance) {
  auto found = _registry.find(msg_type);
  if (found != _registry.end()) {
    return false;
  } else {
    _registry[msg_type] = handler;
    _clearance[msg_type] = minimumClearance;
    return true;
  }
}

void msg::handlers::Registry::dump() {
  Serial.print("Registered message handlers (msg::handlers::Registry): ");
  for(auto const &kv : _registry) {
    Serial.print(kv.first.c_str());
    if(!kv.second) Serial.print("(NULLPTR!)");
    Serial.print(" ");
  }
  Serial.println("");
  Serial.println("Registry clearance levels:");
  for(auto const &kv : _clearance) {
    Serial.print(kv.first.c_str()); Serial.print(":"); Serial.print((int)(kv.second)); Serial.print(" ");
  }
  Serial.println();
}

void msg::handlers::Registry::write_handler_names_to(JsonArray &target) {
  for(auto const &kv : _registry) {
    if(kv.second) target.add(kv.first);
  }
}

bool msg::handlers::PingRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["now"] = "2007-08-31T16:47+01:00";
  // Note, with some initial NTP call we could get micro-second time resolution if we need it
  // for whatever reason.
  msg_out["micros"] = micros();
  return false;
}

bool msg::handlers::GetSystemStatus::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto odist = msg_out.createNestedObject("dist");
  dist::write_to_json(odist, /* include_secrets */ false);

  auto oauth = msg_out.createNestedObject("auth");
  _auth.status(oauth);

  auto oeth = msg_out.createNestedObject("ethernet");
  ethernet::status(oeth);

  return true;
}

bool msg::handlers::HelpHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["human_readable_info"] = "This is a JSON-Lines protocol described at https://anabrid.dev/docs/pyanabrid-redac/redac/protocol.html";

  auto types_list = msg_out.createNestedArray("available_types");
  msg::handlers::Registry::write_handler_names_to(types_list);

  return true;
}
