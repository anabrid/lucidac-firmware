#pragma once

#include "net/ethernet.h"
#include "net/auth.h"
#include "nvmconfig/persistent.h"
#include "protocol/handler.h"

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class GetNetworkSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    // could get string handles as in net::StartupConfig::get().name()
    msg_out["eth"] = net::StartupConfig::get();
    msg_out["auth"] = net::auth::Gatekeeper::get();
    return success;
  }
};

/// @ingroup MessageHandlers
class SetNetworkSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    if(msg_in.containsKey("eth"))
      net::StartupConfig::get().fromJson(msg_in["eth"]);
    if(msg_in.containsKey("auth"))
      net::auth::Gatekeeper::get().fromJson(msg_in["auth"]);

    // Check whether register_for_write() was called in all all
    //  fromJson assignments!
    // probably call by hand...

    nvmconfig::PersistentSettingsWriter::get().write_to_eeprom();
    // TODO find out whether success or not
    return success;
  }
};

/// @ingroup MessageHandlers
class NetworkStatusHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    net::status(msg_out);
    return success;
  }
};

/// @ingroup MessageHandlers
class ResetNetworkSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    // TODO: Must also reset all other config classes!
    //       Probably better reset all nvmconfig info or so.
    net::StartupConfig::get().reset_defaults();
    return success;
  }
};

} // namespace handlers

} // namespace msg