// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "protocol/registry.h"

#include "utils/logging.h"

#include "handlers/carrier.h"
#include "handlers/daq.h"
#include "handlers/help.h"
#include "handlers/ident.h"
#include "handlers/loader_flasher.h"
#include "handlers/loader_plugin.h"
#include "handlers/mode_manual.h"
#include "handlers/net_settings.h"
#include "handlers/ping.h"
#include "handlers/reboot.h"
#include "handlers/run_manager.h"
#include "handlers/login_lock.h"

// allocate singleton storage
msg::handlers::DynamicRegistry msg::handlers::Registry;

void msg::handlers::DynamicRegistry::init(carrier::Carrier& c) {
  using namespace net::auth;
  using namespace msg::handlers;

  // Stateless protocol basics
  set("ping",                 100, new PingRequestHandler{}, SecurityLevel::RequiresNothing);
  set("help",                 200, new HelpHandler(), SecurityLevel::RequiresNothing);

  // Carrier and RunManager things
  set("reset_circuit",        300, new ResetRequestHandler(c), SecurityLevel::RequiresLogin);
  set("set_circuit",          400, new SetConfigMessageHandler(c), SecurityLevel::RequiresLogin);
  set("get_circuit",          500, new GetConfigMessageHandler(c), SecurityLevel::RequiresLogin);
  set("get_entities",         600, new GetEntitiesRequestHandler(c), SecurityLevel::RequiresLogin);
  set("start_run",            700, new StartRunRequestHandler(run::RunManager::get()), SecurityLevel::RequiresLogin);

  // manual hardware access
  set("one_shot_daq",         800, new OneshotDAQHandler(), SecurityLevel::RequiresNothing);
  set("manual_mode",          900, new ManualControlHandler(), SecurityLevel::RequiresNothing);

  set("net_get",             6000, new GetNetworkSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("net_set",             6100, new SetNetworkSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("net_reset",           6200, new ResetNetworkSettingsHandler(), SecurityLevel::RequiresAdmin);
  set("net_status",          6300, new NetworkStatusHandler(), SecurityLevel::RequiresNothing);
  // ^ this net_status won't contain sensitive information...

  //set("status",            3000, new GetSystemStatus(), SecurityLevel::RequiresNothing);
  set("login",               3100, new LoginHandler(), SecurityLevel::RequiresNothing);
  set("lock_acquire",        3200, new LockAcquire(), SecurityLevel::RequiresLogin);
  set("lock_release",        3300, new LockRelease(), SecurityLevel::RequiresLogin);

  set("sys_ident",           3400, new GetSystemIdent(), SecurityLevel::RequiresNothing);
  set("sys_reboot",          3500, new RebootHandler(), SecurityLevel::RequiresAdmin);

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

net::auth::SecurityLevel msg::handlers::DynamicRegistry::requiredClearance(const std::string &msg_type) {
  auto found = entries.find(msg_type);
  if (found != entries.end()) {
    return found->second.clearance;
  } else {
    return net::auth::SecurityLevel::RequiresNothing;
  }
}

bool msg::handlers::DynamicRegistry::set(const std::string &msg_type,
                                  msg::handlers::MessageHandler *handler,
                                  net::auth::SecurityLevel minimumClearance) {
  return set(msg_type, result_code_counter, handler, minimumClearance);
}

bool msg::handlers::DynamicRegistry::set(const std::string &msg_type, int result_code_prefix,
                                  msg::handlers::MessageHandler *handler,
                                  net::auth::SecurityLevel minimumClearance) {
  auto found = entries.find(msg_type);
  if (found != entries.end()) {
    return false;
  } else {
    entries[msg_type] = msg::handlers::DynamicRegistry::RegistryEntry{ handler, minimumClearance };
    handler->result_prefix = result_code_prefix;
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
