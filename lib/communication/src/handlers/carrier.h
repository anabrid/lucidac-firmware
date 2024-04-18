// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>

#include "carrier/carrier.h"
#include "protocol/handler.h"

namespace msg {
namespace handlers {

using namespace carrier;

class CarrierMessageHandlerBase : public MessageHandler {
protected:
  Carrier &carrier;

public:
  explicit CarrierMessageHandlerBase(Carrier &carrier);
};

class SetConfigMessageHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class GetConfigMessageHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class GetEntitiesRequestHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class ResetRequestHandler : public CarrierMessageHandlerBase {
public:
  using CarrierMessageHandlerBase::CarrierMessageHandlerBase;

  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers
} // namespace msg
