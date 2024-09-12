// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "protocol/handler.h"

#include "carrier/carrier.h"
#include "ota/flasher.h" // reboot()

namespace msg {
namespace handlers {

using namespace carrier;

/// @ingroup MessageHandlers
class CarrierMessageHandlerBase : public msg::handlers::MessageHandler {
protected:
  Carrier &carrier;

public:
  explicit CarrierMessageHandlerBase(Carrier &carrier) : carrier(carrier) {}
};

/// @ingroup MessageHandlers
class SetConfigMessageHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    utils::status result = carrier.user_set_config(msg_in, msg_out);
    if(!result)
      msg_out["error"] = result.msg;
    return error(result.code);
    // TODO TODO also expose carrier.user_set_calibrated_config !! 
  }
};

/// @ingroup MessageHandlers
class GetConfigMessageHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    utils::status result = carrier.user_get_config(msg_in, msg_out);
    if(!result)
      msg_out["error"] = result.msg;
    return error(result.code);
  }
};

/// @ingroup MessageHandlers
class GetEntitiesRequestHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    auto entities_obj = msg_out.createNestedObject("entities");
    carrier.classifier_to_json(entities_obj);
    return success;
  }
};

/// @ingroup MessageHandlers
class ResetRequestHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    utils::status result = carrier.user_reset_config(msg_in, msg_out);
    if(!result)
      msg_out["error"] = result.msg;
    return error(result.code);
  }
};

} // namespace handlers
} // namespace msg
