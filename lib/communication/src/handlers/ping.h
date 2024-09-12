// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "protocol/handler.h"
#include "utils/error.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class PingRequestHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    msg_out["now"] = "2007-08-31T16:47+01:00";
    // Note, with some initial NTP call we could get micro-second time resolution if we need it
    // for whatever reason.
    msg_out["micros"] = micros();

    if(msg_in.containsKey("test_ret")) {
      utils::status test("Formatted %s with %d", "string", 123);
      msg_out["error"] = test.msg;
      return test.code;
    }
    return success;
  }
};

} // namespace handlers

} // namespace msg
