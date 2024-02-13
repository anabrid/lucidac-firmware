// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "protocol/registry.h"

#include "utils/logging.h"

#include "handlers/carrier.h"
#include "handlers/daq.h"
#include "handlers/help.h"
#include "handlers/loader_flasher.h"
#include "handlers/loader_plugin.h"
#include "handlers/ping.h"
#include "handlers/run_manager.h"
#include "handlers/status.h"
#include "handlers/user_login.h"
#include "handlers/user_settings.h"
#include "handlers/mode_manual.h"

// allocate singleton storage
msg::handlers::DynamicRegistry msg::handlers::Registry;

void msg::handlers::DynamicRegistry::init() {
  using namespace user::auth;
  using namespace msg::handlers;

  // Basics
  set("ping",                 100, new PingRequestHandler{}, SecurityLevel::RequiresNothing);
  set("help",                 200, new HelpHandler(), SecurityLevel::RequiresNothing);
  set("reset",                300, new ResetRequestHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin); // TODO: Should probably be called "reset_config" or so, cf. reset_settings

  // Carrier and RunManager things
  set("set_config",           400, new SetConfigMessageHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("get_config",           500, new GetConfigMessageHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("get_entities",         600, new GetEntitiesRequestHandler(carrier::Carrier::get()), SecurityLevel::RequiresLogin);
  set("start_run",            700, new StartRunRequestHandler(run::RunManager::get()), SecurityLevel::RequiresLogin);

  // manual hardware access
  set("one_shot_daq",         800, new OneshotDAQHandler(), SecurityLevel::RequiresNothing);
  set("manual_mode",          900, new ManualControlHandler(), SecurityLevel::RequiresNothing);


  // TODO: It would be somewhat cleaner if the Hybrid Controller settings would be just part of the get_config/set_config idiom
  //   because with this notation, we double the need for setters and getters.
  set("get_settings",        2000, new GetSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("update_settings",     2100, new SetSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("reset_settings",      2200, new ResetSettingsHandler(), SecurityLevel::RequiresAdmin);

  set("status",              3000, new GetSystemStatus(user::UserSettings.auth), SecurityLevel::RequiresNothing);
  set("login",               3100, new LoginHandler(user::UserSettings.auth), SecurityLevel::RequiresNothing);

  set("load_plugin",         4100, new LoadPluginHandler(), SecurityLevel::RequiresAdmin);
  set("unload_plugin",       4200, new UnloadPluginHandler(), SecurityLevel::RequiresAdmin);

  set("ota_update_init",     5000, new FlasherInitHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_stream",   5100, new FlasherDataHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_abort",    5200, new FlasherAbortHandler(), SecurityLevel::RequiresAdmin);
  set("ota_update_complete", 5300, new FlasherCompleteHandler(), SecurityLevel::RequiresAdmin);
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
