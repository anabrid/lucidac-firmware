// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "carrier.h"

carrier::Carrier::Carrier(std::vector<Cluster> clusters) : Entity(""), clusters(std::move(clusters)) {}

entities::EntityClass carrier::Carrier::get_entity_class() const { return entities::EntityClass::CARRIER; }

bool carrier::Carrier::init(std::string mac_addr) {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  entity_id = mac_addr;
  if (entity_id.empty())
    return false;
  for (auto &cluster : clusters) {
    if (!cluster.init())
      return false;
  }
  return true;
}

std::vector<entities::Entity *> carrier::Carrier::get_child_entities() {
  std::vector<entities::Entity *> children;
  for (auto &cluster : clusters) {
    children.push_back(&cluster);
  }
  return children;
}

entities::Entity *carrier::Carrier::get_child_entity(const std::string &child_id) {
  auto cluster_idx = std::stoul(child_id);
  if (cluster_idx < 0 or clusters.size() < cluster_idx)
    return nullptr;
  return &clusters[cluster_idx];
}

bool carrier::Carrier::config_self_from_json(JsonObjectConst cfg) {
  // TODO: Have an option to fail on unexpected configuration
  return true;
}

void carrier::Carrier::write_to_hardware() {
  for (auto &cluster : clusters) {
    cluster.write_to_hardware();
  }
}

constexpr int error(int i) { return i; } // just some syntactic suggar
constexpr int success = 0;

int carrier::Carrier::set_config(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto self_entity_id = get_entity_id();
  if (!msg_in.containsKey("entity") or !msg_in.containsKey("config")) {
    msg_out["error"] = "Malformed message.";
    return error(1);
  }

  // Convert JSON array of possible anything to string array
  auto path_json = msg_in["entity"].as<JsonArrayConst>();
  auto path_depth = path_json.size();
  std::string path[path_depth];
  copyArray(path_json, path, path_depth);

  // Sanity check path, which must at least be addressed to us
  if (!path_depth) {
    msg_out["error"] = "Invalid entity path.";
    return error(2);
  }
  if (path[0] != self_entity_id) {
    msg_out["error"] = "Message intended for another carrier.";
    return error(3);
  }

  // Path may be to one of our sub-entities
  auto resolved_entity = resolve_child_entity(path + 1, path_depth - 1);
  if (!resolved_entity) {
    msg_out["error"] = "No entity at that path.";
    return error(4);
  }

  bool write_success = resolved_entity->config_from_json(msg_in["config"]);
  if (!write_success && msg_out["error"].isNull()) {
    // TODO: Never reachable due to msg_out["error"].isNull()
    msg_out["error"] = "Error applying configuration to entity.";
    return error(5);
  }

  // Actually write to hardware
  write_to_hardware();
  return write_success ? success : error(6);
}

int carrier::Carrier::get_config(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto recursive = true;
  if (msg_in.containsKey("recursive"))
    recursive = msg_in["recursive"].as<bool>();

  // Message may contain path to sub-entity
  entities::Entity *entity = nullptr;
  if (!msg_in.containsKey("entity") or msg_in["entity"].isNull()) {
    entity = this;
  } else if (msg_in["entity"].is<JsonArrayConst>()) {
    auto path = msg_in["entity"].as<JsonArrayConst>();
    if (!path.size()) {
      entity = this;
    } else if (path[0].as<std::string>() != get_entity_id()) {
      msg_out["error"] = "Entity lives on another carrier.";
      return error(1);
    } else {
      auto path_begin = path.begin();
      ++path_begin;
      entity = resolve_child_entity(path_begin, path.end());
      if (!entity) {
        msg_out["error"] = "Invalid entity path.";
        return error(2);
      }
    }
  } else {
    msg_out["error"] = "Invalid entity path.";
    return error(3);
  }

  // Save entity path back into response
  msg_out["entity"] = msg_in["entity"];
  // Save config into response
  auto cfg = msg_out.createNestedObject("config");
  entity->config_to_json(cfg, recursive);
  return success;
}

int carrier::Carrier::get_entities(JsonObjectConst msg_in, JsonObject &msg_out) {
  auto entities_obj = msg_out.createNestedObject("entities");
  auto carrier_obj = entities_obj[get_entity_id()] = get_entity_classifier();
  for (const auto &cluster : clusters) {
    auto cluster_obj = carrier_obj["/" + cluster.get_entity_id()] = cluster.get_entity_classifier();
    for (auto *block : cluster.get_blocks()) {
      if (block)
        cluster_obj["/" + block->get_entity_id()] = block->get_entity_classifier();
    }
  }
  return success;
}

int carrier::Carrier::reset(JsonObjectConst msg_in, JsonObject &msg_out) {
  for (auto &cluster : clusters) {
    cluster.reset(msg_in["keep_calibration"] | true);
  }
  if (msg_in["sync"] | true) {
    write_to_hardware();
  }
  return success;
}
