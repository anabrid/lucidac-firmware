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

#include "carrier.h"

#include <QNEthernet.h>

std::string carrier::Carrier::get_system_mac() {
  uint8_t mac[6];
  qindesign::network::Ethernet.macAddress(mac);
  char mac_str[20];
  sprintf(mac_str, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return mac_str;
}

carrier::Carrier::Carrier() : clusters({lucidac::LUCIDAC()}) {}

bool carrier::Carrier::init() {
  entity_id = get_system_mac();
  if (entity_id.empty())
    return false;
  for (auto &cluster : clusters) {
    if (!cluster.init())
      return false;
  }
  return true;
}

const std::string &carrier::Carrier::get_entity_id() const { return entity_id; }

msg::handlers::SetConfigMessageHandler::SetConfigMessageHandler(Carrier &carrier) : carrier(carrier) {}

bool msg::handlers::SetConfigMessageHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto self_entity_id = carrier.get_entity_id();
  Serial.println("carrier::SetConfigMessageHandler::handle()");
  Serial.println(self_entity_id.c_str());
  if (!msg_in.containsKey("entity") or !msg_in.containsKey("config")) {
    msg_out["error"] = "Malformed message.";
    return false;
  }

  auto path = msg_in["entity"].as<JsonArrayConst>();
  if (path.isNull() or path.size() < 1) {
    msg_out["error"] = "Invalid entity path.";
    return false;
  } else if (path[0].as<std::string>() != self_entity_id) {
    msg_out["error"] = "Message intended for someone else.";
    return false;
  }

  // TODO: The message is either directly addressed to the carrier board or to one sub-entity.
  //       In first case, it may contain a nested dictionary configuring multiple entities.
  //       In the latter case, we select the respective entity and pass on the configuration.
  //       For this, create an Entity class which has such functionality.
  return true;
}
