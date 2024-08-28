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

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class GetSystemIdent : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    msg_out["serial"] = nvmconfig::VendorOTP().serial_number;

    dist::write_to_json(msg_out.createNestedObject("fw_build"));
    loader::flashimage::toJson(msg_out.createNestedObject("fw_image"));
    return success;
  }
};

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

} // namespace handlers
} // namespace msg
