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

#include "entity.h"
#include "cluster.h"
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

  Entity *get_child_entity(const std::string &child_id) override;
  bool config_from_json(JsonObjectConst cfg) override;
};

} // namespace carrier

namespace msg {
namespace handlers {

using namespace carrier;

class SetConfigMessageHandler : public MessageHandler {
  Carrier &carrier;

public:
  explicit SetConfigMessageHandler(Carrier &carrier);
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers
} // namespace msg