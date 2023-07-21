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

carrier::Carrier::Carrier() : clusters({lucidac::LUCIDAC(0)}) {}

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

bool carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
  Serial.println(__PRETTY_FUNCTION__);
#endif
  // Carrier has no own configuration parameters currently
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

entities::Entity *carrier::Carrier::get_child_entity(const std::string &child_id) {
  // TODO: Use additional arguments of stoul to check if only part of the string was used
  auto cluster_idx = std::stoul(child_id);
  if (cluster_idx < 0 or clusters.size() < cluster_idx)
    return nullptr;
  return &clusters[cluster_idx];
}

msg::handlers::SetConfigMessageHandler::SetConfigMessageHandler(Carrier &carrier) : carrier(carrier) {}

bool msg::handlers::SetConfigMessageHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto self_entity_id = carrier.get_entity_id();
  if (!msg_in.containsKey("entity") or !msg_in.containsKey("config")) {
    msg_out["error"] = "Malformed message.";
    return false;
  }

  // Convert JSON array of possible anything to string array
  auto path_json = msg_in["entity"].as<JsonArrayConst>();
  auto path_depth = path_json.size();
  std::string path[path_depth];
  copyArray(path_json, path, path_depth);

  // Sanity check path, which must at least be addressed to us
  if (!path_depth) {
    msg_out["error"] = "Invalid entity path.";
    return false;
  }
  if (path[0] != self_entity_id) {
    msg_out["error"] = "Message intended for another carrier.";
    return false;
  }

  // Path may be to one of our sub-entities
  auto resolved_entity = carrier.resolve_child_entity(path + 1, path_depth - 1);
  if (!resolved_entity) {
    msg_out["error"] = "No entity at that path.";
    return false;
  }

  return resolved_entity->config_from_json(msg_in["config"]);
}
