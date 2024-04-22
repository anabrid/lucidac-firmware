// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <string>
#include <vector>

#include <Arduino.h>
#include <ArduinoJson.h>

namespace entities {

class Entity {
protected:
  std::string entity_id;

public:
  Entity() = default;

  explicit Entity(std::string entityId) : entity_id(std::move(entityId)) {}

  const std::string &get_entity_id() const { return entity_id; }

  virtual std::vector<Entity *> get_child_entities() = 0;

  virtual Entity *get_child_entity(const std::string &child_id) = 0;

  Entity *resolve_child_entity(std::string paths[], size_t len) {
    auto resolved_entity = this;
    for (size_t path_depth = 0; path_depth < len; path_depth++) {
      resolved_entity = resolved_entity->get_child_entity(paths[path_depth]);
      if (!resolved_entity) {
        return nullptr;
      }
    }
    return resolved_entity;
  }

  Entity *resolve_child_entity(JsonArrayConstIterator begin, JsonArrayConstIterator end) {
    auto resolved_entity = this;
    for (auto sub_path = begin; sub_path != end; ++sub_path) {
      std::string child_entity_id = (*sub_path).as<const char *>();
      resolved_entity = resolved_entity->get_child_entity(child_entity_id);
      if (!resolved_entity) {
        return nullptr;
      }
    }
    return resolved_entity;
  }

  Entity *resolve_child_entity(JsonArrayConst path) { return resolve_child_entity(path.begin(), path.end()); }

  bool config_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
    Serial.println(__PRETTY_FUNCTION__);
#endif
    if (cfg.isNull())
      return false;
    if (!config_self_from_json(cfg))
      return false;
    if (!config_children_from_json(cfg))
      return false;
    return true;
  }

  void config_to_json(JsonObject &cfg, bool recursive = true) {
    config_self_to_json(cfg);
    if (recursive)
      config_children_to_json(cfg);
  }

protected:
  virtual bool config_self_from_json(JsonObjectConst cfg) = 0;

  bool config_children_from_json(JsonObjectConst &cfg) {
    for (JsonPairConst keyval : cfg) {
      if (keyval.key().c_str()[0] == '/' and keyval.key().size() > 1) {
        std::string child_id(keyval.key().c_str() + 1);
        auto child_entity = get_child_entity(child_id);
        if (!child_entity)
          return false;
        if (!child_entity->config_from_json(keyval.value()))
          return false;
      }
    }
    return true;
  }

  virtual void config_self_to_json(JsonObject &cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
    Serial.println(__PRETTY_FUNCTION__);
#endif
  }

  void config_children_to_json(JsonObject &cfg) {
    for (auto child : get_child_entities()) {
      auto child_cfg = cfg.createNestedObject(std::string("/") + child->get_entity_id());
      child->config_to_json(child_cfg, true);
    }
  }
};

} // namespace entities