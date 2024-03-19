// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>

#include "cluster.h"
#include "entity/entity.h"
#include "protocol/handler.h"

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

  // REV1 specific things
  // TODO: These are partly LUCIDAC specific things, which should be rebased on `56-refactor-...` branch.
public:
  // Module addresses
  static constexpr uint8_t CARRIER_MADDR = 5;
  static constexpr uint8_t CTRL_MADDR = 2;
  static constexpr uint8_t BACKPLANE_MADDR = 2;
  static constexpr uint8_t FRONTPLANE_MADDR = 2;
  // Function addresses
  static constexpr uint8_t METADATA_FADDR = bus::METADATA_FUNC_IDX;
  static constexpr uint8_t TEMPERATURE_FADDR = 1;
  static constexpr uint8_t ADC_PRG_FADDR = 2;
  static constexpr uint8_t ADC_RESET_8816_FADDR = 3;
  static constexpr uint8_t ADC_RESET_SR_FADDR = 4;
  static constexpr uint8_t ADC_STROBE_FADDR = 5;
  static constexpr uint8_t ACL_PRG_FADDR = 6;
  static constexpr uint8_t ACL_UPD_FADDR = 7;
  static constexpr uint8_t ACL_CRL_FADDR = 8;
protected:
  const functions::SR74HCT595 f_acl_prg;
  const functions::TriggerFunction f_acl_upd;
  const functions::TriggerFunction f_acl_clr;
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