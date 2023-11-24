// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>

#include "cluster.h"
#include "entity.h"
#include "message_handlers.h"

using namespace lucidac;

namespace carrier {

class Carrier : public entities::Entity {
public:
  static std::string get_system_mac();

public:
  std::array<LUCIDAC, 1> clusters;

  Carrier();

  bool init();

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

  void write_to_hardware();
};

} // namespace carrier

namespace msg {
namespace handlers {

using namespace carrier;

class CarrierMessageHandlerBase : public msg::handlers::MessageHandler {
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