// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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
    return error(carrier.set_config(msg_in, msg_out));
  }
};

/// @ingroup MessageHandlers
class GetConfigMessageHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(carrier.get_config(msg_in, msg_out));
  }
};

/// @ingroup MessageHandlers
class GetEntitiesRequestHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(carrier.get_entities(msg_in, msg_out));
  }
};

/// @ingroup MessageHandlers
class ResetRequestHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    // TODO: resetting the Teensy itself and not only the carrier should be executed somewhere more suitable.
    if(msg_in["reboot"]) {
      loader::reboot(); // does actually not return
      return success;
    } else {
      return error(carrier.reset(msg_in, msg_out));
    }
  }
};

} // namespace handlers
} // namespace msg