// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "message_registry.h"

#include "user_ethernet.h"
#include "user_auth.h"
#include "distributor.h"
#include "hashflash.h"
#include "plugin.h"
#include "flasher.h"

#include "distributor.h"
#include "carrier.h"
#include "client.h"
#include "logging.h"
#include "message_registry.h"
#include "run.h"
#include "user_settings.h"
#include "serial_lines.h"
#include "user_auth.h"
#include "user_login.h"
#include "run_manager.h"

#include "hashflash.h"
#include "plugin.h"
#include "flasher.h"

// allocate singleton storage
msg::handlers::DynamicRegistry msg::handlers::Registry;

void msg::handlers::DynamicRegistry::init() {
  using namespace user::auth;
  using namespace msg::handlers;

  // Register message handler
  set("ping", new PingRequestHandler{}, SecurityLevel::RequiresNothing);
  set("help", new HelpHandler(), SecurityLevel::RequiresNothing);
  set("reset", new ResetRequestHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin); // TODO: Should probably be called "reset_config" or so, cf. reset_settings
  set("set_config", new SetConfigMessageHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("get_config", new GetConfigMessageHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("get_entities", new GetEntitiesRequestHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("start_run", new StartRunRequestHandler(run::RunManager::get()), SecurityLevel::RequiresLogin);

  // TODO: It would be somewhat cleaner if the Hybrid Controller settings would be just part of the get_config/set_config idiom
  //   because with this notation, we double the need for setters and getters.
  set("get_settings", new GetSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("update_settings", new SetSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("reset_settings", new ResetSettingsHandler(), SecurityLevel::RequiresAdmin);

  set("status", new GetSystemStatus(user::UserSettings.auth), SecurityLevel::RequiresNothing);
  set("login", new LoginHandler(user::UserSettings.auth), SecurityLevel::RequiresNothing);

  set("load_plugin", new LoadPluginHandler(), SecurityLevel::RequiresAdmin);
  set("unload_plugin", new UnloadPluginHandler(), SecurityLevel::RequiresAdmin);

  set("ota_update_init", new FlasherInitHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_stream", new FlasherDataHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_abort", new FlasherAbortHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_complete", new FlasherCompleteHandler(), SecurityLevel::RequiresAdmin);
}

msg::handlers::MessageHandler *msg::handlers::DynamicRegistry::get(const std::string &msg_type) {
  auto found = entries.find(msg_type);
  if (found != entries.end()) {
    return found->second.handler;
  } else {
    return nullptr;
  }
}

user::auth::SecurityLevel msg::handlers::DynamicRegistry::requiredClearance(const std::string &msg_type) {
  auto found = entries.find(msg_type);
  if (found != entries.end()) {
    return found->second.clearance;
  } else {
    return user::auth::SecurityLevel::RequiresNothing;
  }
}

bool msg::handlers::DynamicRegistry::set(const std::string &msg_type, msg::handlers::MessageHandler *handler,
                                  user::auth::SecurityLevel minimumClearance) {
  auto found = entries.find(msg_type);
  if (found != entries.end()) {
    return false;
  } else {
    entries[msg_type] = msg::handlers::DynamicRegistry::RegistryEntry{ handler, minimumClearance };
    return true;
  }
}

void msg::handlers::DynamicRegistry::dump() {
  Serial.print("Registered message handlers (msg::handlers::DynamicRegistry): ");
  for(auto const &kv : entries) {
    Serial.print(kv.first.c_str());
    if(!kv.second.handler) Serial.print("(NULLPTR!)");
    Serial.print(" ");
  }
  Serial.println("");
  Serial.println("Registry clearance levels:");
  for(auto const &kv : entries) {
    Serial.print(kv.first.c_str()); Serial.print(":"); Serial.print((int)(kv.second.clearance)); Serial.print(" ");
  }
  Serial.println();
}

void msg::handlers::DynamicRegistry::write_handler_names_to(JsonArray &target) {
  for(auto const &kv : entries) {
    if(kv.second.handler) target.add(kv.first);
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
  bool do_all = msg_in.size()==0;

  if(do_all || msg_in["dist"].as<bool>()) {
    auto odist = msg_out.createNestedObject("dist");
    dist::write_to_json(odist, /* include_secrets */ false);
  }

  if(do_all || msg_in["flashimage"].as<bool>()) {
    msg_out["flashimage"] = loader::flashimage();
  }

  if(do_all || msg_in["auth"].as<bool>()) {
    auto oauth = msg_out.createNestedObject("auth");
    _auth.status(oauth);
  }

  if(do_all || msg_in["ethernet"].as<bool>()) {
    auto oeth = msg_out.createNestedObject("ethernet");
    user::ethernet::status(oeth);
  }

  if(do_all || msg_in["plugin"].as<bool>()) {
    auto loader_list = msg_out["plugin"].createNestedArray("loaders");
    loader_list.add(loader::PluginLoader);
  }

  if(do_all || msg_in["flasher"].as<bool>()) {
    auto oflasher = msg_out.createNestedObject("flasher");
    loader::FirmwareBuffer::status(oflasher);
  }

  return true;
}

bool msg::handlers::HelpHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["human_readable_info"] = "This is a JSON-Lines protocol described at https://anabrid.dev/docs/pyanabrid-redac/redac/protocol.html";

  auto types_list = msg_out.createNestedArray("available_types");
  msg::handlers::Registry.write_handler_names_to(types_list);

  return true;
}

