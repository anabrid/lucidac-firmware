// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "build/distributor.h"
#include "nvmconfig/vendor.h"
#include "ota/flasher.h" // reboot()
#include "protocol/handler.h"
#include "protocol/jsonl_logging.h"
#include "utils/hashflash.h"
#include "lucidac/front_panel_signaling.h"
#include "mode/counters.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class GetSystemIdent : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext &user_context) override {
    msg_out["serial"] = nvmconfig::VendorOTP::get().serial_number;
    msg_out["mac"] = net::StartupConfig::get().mac;

    if(msg_in.containsKey("blink")) {
      leds::indicate_error();// this is just blocking blinking for a few seconds
    }


    dist::write_to_json(msg_out.createNestedObject("fw_build"));
    loader::flashimage::toJson(msg_out.createNestedObject("fw_image"));
    return success;
  }
};

#ifdef ANABRID_WRITE_EEPROM

/// @ingroup MessageHandlers
class WriteSystemIdent : public MessageHandler {
public:
    int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      auto& persistent = nvmconfig::PersistentSettingsWriter::get();
      auto& vendor = nvmconfig::VendorOTP::get();
      persistent.read_from_eeprom(); // at first run, this will result in default values
      if(msg_in.containsKey("vendor"))
        vendor.fromJson(msg_in["vendor"], nvmconfig::Context::User);
      bool valid = vendor.is_valid();
      if(valid)
        persistent.write_to_eeprom();

      msg_out["valid"] = valid;
      return success;
  }
};

/// @ingroup MessageHandlers
class ResetSystemIdent : public MessageHandler {
public:
    int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      bool write_to_hardware = msg_in["write_to_hardware"];
      auto& persistent = nvmconfig::PersistentSettingsWriter::get();
      persistent.reset_defaults(write_to_hardware);
      return success;
  }
};

/// @ingroup MessageHandlers

class ReadSystemIdent : public MessageHandler {
public:
    int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      auto& persistent = nvmconfig::PersistentSettingsWriter::get();
      if(msg_in.containsKey("read_from_eeprom"))
        persistent.read_from_eeprom();

      // dump infos about all subsystems, including passwords, etc
      auto subsystems = msg_out.createNestedObject("subsystems");
      for(auto const& sys : nvmconfig::PersistentSettingsWriter::get().subsystems) {
        auto sysout = subsystems.createNestedObject(sys->name());
        sys->toJson(sysout, nvmconfig::Context::User);
      }
      
      return success;
  }
};
#endif

/// @ingroup MessageHandlers
class RebootHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    loader::reboot(); // does actually not return
    return success;
  }
};

/// @ingroup MessageHandlers
class SyslogHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, utils::StreamingJson &msg_out) override {
    msg::StartupLog::get().stream_to_json(msg_out);
    return success;
  }
};

class SystemStats : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    auto perf_counters = msg_out.createNestedObject("perf_counters");
    mode::PerformanceCounter::get().to_json(perf_counters);
    return success;
  }
};

} // namespace handlers
} // namespace msg
