// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "message_handlers.h"

#include "persistent_eth.h"
#include "user_auth.h"
#include "distributor.h"
#include "hashflash.h"

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

  auto oflash = msg_out.createNestedObject("flashimage");
  oflash["size"] = utils::flashimagelen();
  oflash["sha256sum"] = utils::sha256_to_string(utils::hash_flash_sha256());

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
