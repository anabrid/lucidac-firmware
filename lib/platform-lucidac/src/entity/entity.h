// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <string>
#include <utility>
#include <vector>

#include <Arduino.h>
#include <ArduinoJson.h>

namespace entities {

enum class EntityClass : uint8_t {
  UNKNOWN = 0,
  CARRIER = 1,
  CLUSTER = 2,
  M_BLOCK = 3,
  U_BLOCK = 4,
  C_BLOCK = 5,
  I_BLOCK = 6,
  SH_BLOCK = 7,
  FRONT_PLANE = 8
};

typedef struct __attribute__((packed)) EntityClassifier {
  union {
    const uint8_t class_;
    const EntityClass class_enum;
  };

  const uint8_t type;
  const uint8_t variant;
  const uint8_t version;

  static constexpr uint8_t UNKNOWN = 0;
  static constexpr uint8_t DEFAULT_ = 1;

  EntityClassifier(const EntityClass class_, const uint8_t type_, const uint8_t variant_,
                   const uint8_t version_)
      : class_enum(class_), type(type_), variant(variant_), version(version_) {}

  EntityClassifier(const uint8_t class_, const uint8_t type_, const uint8_t variant_, const uint8_t version_)
      : class_(class_), type(type_), variant(variant_), version(version_) {}

  template <class EntityType_> EntityType_ type_as() const { return static_cast<EntityType_>(type); }

  template <class EntityVariant_> EntityVariant_ variant_as() const {
    return static_cast<EntityVariant_>(variant);
  }

  template <class EntityVersion_> EntityVersion_ version_as() const {
    return static_cast<EntityVersion_>(version);
  }

  std::string to_string() const {
    return "(" + std::to_string(class_) + ", " + std::to_string(type) + ", " + std::to_string(variant) + ", " +
           std::to_string(version) + ")";
  }

  explicit operator bool() const {
    // All fields of a valid EntityClassifier must not be zero.
    return class_ and type and version and variant;
  }

} EntityClassifier;

static_assert(sizeof(EntityClassifier) == 4, "EntityClassifier has unexpected number of bytes.");

class Entity {
protected:
  std::string entity_id;

public:
  Entity() = default;

  explicit Entity(std::string entityId) : entity_id(std::move(entityId)) {}

  const std::string &get_entity_id() const { return entity_id; }

  virtual EntityClassifier get_entity_classifier() const {
    return {get_entity_class(), get_entity_type(), get_entity_variant(), get_entity_version()};
  }

  virtual EntityClass get_entity_class() const = 0;

  virtual uint8_t get_entity_type() const {
    // For many entities, there will only be one default type.
    return EntityClassifier::DEFAULT_;
  }

  virtual uint8_t get_entity_version() const {
    // For many entities, there will only be one default version.
    return EntityClassifier::DEFAULT_;
  }

  virtual uint8_t get_entity_variant() const {
    // For many entities, there will only be one default variation.
    return EntityClassifier::DEFAULT_;
  }

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

namespace ArduinoJson {

template <> struct Converter<entities::EntityClassifier> {
  static bool toJson(const entities::EntityClassifier &src, JsonVariant dst) {
    dst["class"] = src.class_;
    dst["type"] = src.type;
    dst["variant"] = src.variant;
    dst["version"] = src.version;
    return true;
  }

  static entities::EntityClassifier fromJson(JsonVariantConst src) {
    return {src["class"].as<uint8_t>(), src["type"], src["variant"], src["version"]};
  }

  static bool checkJson(JsonVariantConst src) {
    return src["class"].is<uint8_t>() and src["type"].is<uint8_t>() and src["variant"].is<uint8_t>() and
           src["version"].is<uint8_t>();
  }
};

} // namespace ArduinoJson
