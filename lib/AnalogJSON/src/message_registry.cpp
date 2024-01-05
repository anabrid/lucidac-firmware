// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "message_registry.h"

#include "user/user_ethernet.h"
#include "user/user_auth.h"
#include "user/user_settings.h"
#include "user/user_auth.h"
#include "dist/distributor.h"
#include "loader/hashflash.h"
#include "loader/plugin.h"
#include "loader/flasher.h"

#include "carrier/carrier.h"
#include "client/client.h"
#include "run/run.h"
#include "run/run_manager.h"

#include "utils/serial_lines.h"
#include "utils/logging.h"

#include "handlers/loader_flasher.h"
#include "handlers/loader_plugin.h"
#include "handlers/user_login.h"
#include "handlers/user_settings.h"


// allocate singleton storage
msg::handlers::DynamicRegistry msg::handlers::Registry;

void msg::handlers::DynamicRegistry::init() {
  using namespace user::auth;
  using namespace msg::handlers;

  // Register message handler
  set("ping",                 100, new PingRequestHandler{}, SecurityLevel::RequiresNothing);
  set("help",                 200, new HelpHandler(), SecurityLevel::RequiresNothing);
  set("reset",                300, new ResetRequestHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin); // TODO: Should probably be called "reset_config" or so, cf. reset_settings
  set("set_config",           400, new SetConfigMessageHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("get_config",           500, new GetConfigMessageHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("get_entities",         600, new GetEntitiesRequestHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("start_run",            700, new StartRunRequestHandler(run::RunManager::get()), SecurityLevel::RequiresLogin);

  // TODO: It would be somewhat cleaner if the Hybrid Controller settings would be just part of the get_config/set_config idiom
  //   because with this notation, we double the need for setters and getters.
  set("get_settings",        1000, new GetSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("update_settings",     1100, new SetSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("reset_settings",      1200, new ResetSettingsHandler(), SecurityLevel::RequiresAdmin);

  set("status",              2000, new GetSystemStatus(user::UserSettings.auth), SecurityLevel::RequiresNothing);
  set("login",               2100, new LoginHandler(user::UserSettings.auth), SecurityLevel::RequiresNothing);

  set("load_plugin",         3100, new LoadPluginHandler(), SecurityLevel::RequiresAdmin);
  set("unload_plugin",       3200, new UnloadPluginHandler(), SecurityLevel::RequiresAdmin);

  set("ota_update_init",     4000, new FlasherInitHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_stream",   4100, new FlasherDataHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_abort",    4200, new FlasherAbortHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_complete", 4300, new FlasherCompleteHandler(), SecurityLevel::RequiresAdmin);
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

bool msg::handlers::DynamicRegistry::set(const std::string &msg_type,
                                  msg::handlers::MessageHandler *handler,
                                  user::auth::SecurityLevel minimumClearance) {
  return set(msg_type, result_code_counter, handler, minimumClearance);
}

bool msg::handlers::DynamicRegistry::set(const std::string &msg_type, int result_code_prefix,
                                  msg::handlers::MessageHandler *handler,
                                  user::auth::SecurityLevel minimumClearance) {
  auto found = entries.find(msg_type);
  if (found != entries.end()) {
    return false;
  } else {
    entries[msg_type] = msg::handlers::DynamicRegistry::RegistryEntry{
      handler, minimumClearance, result_code_prefix };
    result_code_counter = result_code_prefix + result_code_increment;
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

int msg::handlers::PingRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["now"] = "2007-08-31T16:47+01:00";
  // Note, with some initial NTP call we could get micro-second time resolution if we need it
  // for whatever reason.
  msg_out["micros"] = micros();
  return success;
}

int msg::handlers::GetSystemStatus::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
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
    loader::FirmwareFlasher::get().status(oflasher);
  }

  return success;
}

int msg::handlers::HelpHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["human_readable_info"] = "This is a JSON-Lines protocol described at https://anabrid.dev/docs/pyanabrid-redac/redac/protocol.html";

  auto types_list = msg_out.createNestedArray("available_types");
  msg::handlers::Registry.write_handler_names_to(types_list);

  return success;
}

