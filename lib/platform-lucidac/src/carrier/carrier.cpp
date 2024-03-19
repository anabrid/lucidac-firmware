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
#include "logging.h"
#include "mac_utils.h"

std::string carrier::Carrier::get_system_mac() {
  return ::get_system_mac();
}

carrier::Carrier::Carrier() : clusters({lucidac::LUCIDAC(0)}),
                              f_acl_prg(bus::address_from_tuple(CARRIER_MADDR, ACL_PRG_FADDR), true),
                              f_acl_upd(bus::address_from_tuple(CARRIER_MADDR, ACL_UPD_FADDR)),
                              f_acl_clr(bus::address_from_tuple(CARRIER_MADDR, ACL_CRL_FADDR))
{}

bool carrier::Carrier::init() {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  entity_id = get_system_mac();
  LOG(ANABRID_DEBUG_INIT, entity_id.c_str());
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

std::vector<entities::Entity *> carrier::Carrier::get_child_entities() {
#ifdef ANABRID_DEBUG_ENTITY
  Serial.println(__PRETTY_FUNCTION__);
#endif
  std::vector<entities::Entity *> children;
  for (auto &cluster : clusters) {
    children.push_back(&cluster);
  }
  return children;
}

void carrier::Carrier::write_to_hardware() {
  for (auto &cluster : clusters) {
    cluster.write_to_hardware();
  }
}

msg::handlers::CarrierMessageHandlerBase::CarrierMessageHandlerBase(Carrier &carrier) : carrier(carrier) {}

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

  bool success = resolved_entity->config_from_json(msg_in["config"]);
  if (!success and msg_out["error"].isNull()) {
    msg_out["error"] = "Error applying configuration to entity.";
  }

  // Actually write to hardware
  carrier.write_to_hardware();
  return success;
}

bool msg::handlers::GetConfigMessageHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto recursive = true;
  if (msg_in.containsKey("recursive"))
    recursive = msg_in["recursive"].as<bool>();

  // Message may contain path to sub-entity
  entities::Entity *entity = nullptr;
  if (!msg_in.containsKey("entity") or msg_in["entity"].isNull()) {
    entity = &carrier;
  } else if (msg_in["entity"].is<JsonArrayConst>()) {
    auto path = msg_in["entity"].as<JsonArrayConst>();
    if (!path.size()) {
      entity = &carrier;
    } else if (path[0].as<std::string>() != carrier.get_entity_id()) {
      msg_out["error"] = "Entity lives on another carrier.";
      return false;
    } else {
      auto path_begin = path.begin();
      ++path_begin;
      entity = carrier.resolve_child_entity(path_begin, path.end());
      if (!entity) {
        msg_out["error"] = "Invalid entity path.";
        return false;
      }
    }
  } else {
    msg_out["error"] = "Invalid entity path.";
    return false;
  }

  // Save entity path back into response
  msg_out["entity"] = msg_in["entity"];
  // Save config into response
  auto cfg = msg_out.createNestedObject("config");
  entity->config_to_json(cfg, recursive);
  return true;
}

bool msg::handlers::GetEntitiesRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto serialized_data = R"({"00-00-00-00-00-00": {"/0": {"/M0": {"class": 2, "type": 0, "variant": 0, "version": 0}, "/M1": {"class": 2, "type": 1, "variant": 0, "version": 0}, "/U": {"class": 3, "type": 0, "variant": 0, "version": 0}, "/C": {"class": 4, "type": 0, "variant": 0, "version": 0}, "/I": {"class": 5, "type": 0, "variant": 0, "version": 0}, "class": 1, "type": 3, "variant": 0, "version": 0}, "class": 0, "type": 0, "variant": 0, "version": 0}})";
  std::memcpy((void*)(serialized_data + 2), (void*)(carrier.get_entity_id().c_str()), 17);
  msg_out["entities"] = serialized(serialized_data);
  return true;
}

bool msg::handlers::ResetRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  for (auto &cluster : carrier.clusters) {
    cluster.reset(msg_in["keep_calibration"] | true);
  }
  if (msg_in["sync"] | true) {
    carrier.write_to_hardware();
  }
  return true;
}
