/*

#include "handlers/status.h"

#include <Arduino.h>
#include "net/ethernet.h"
#include "net/auth.h"
#include "build/distributor.h"
#include "plugin/plugin.h"
#include "ota/flasher.h"

int msg::handlers::GetSystemStatus::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  bool do_all = msg_in.size()==0;

  if(do_all || msg_in["time"].as<bool>()) {
    msg_out["time"]["uptime_millis"] = millis();
  }

  if(do_all || msg_in["dist"].as<bool>()) {
    auto odist = msg_out.createNestedObject("dist");
    dist::write_to_json(odist,  false); // include secrets = false
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
    net::ethernet::status(oeth);
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

*/