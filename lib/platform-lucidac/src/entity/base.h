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

#include "version.h"
#include "utils/error.h"

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
  FRONT_PANEL = 8,
  CTRL_BLOCK = 9
};

struct __attribute__((packed)) EntityClassifier {
  union {
    const uint8_t class_;
    const EntityClass class_enum;
  };

  const uint8_t type;
  const Version version;
  const uint8_t variant;

  static constexpr uint8_t UNKNOWN = 0;
  static constexpr uint8_t DEFAULT_ = 1;
  static constexpr Version DEFAULT_VERSION_{1, 0, 0};

  EntityClassifier(const EntityClass class_, const uint8_t type_, const Version version_,
                   const uint8_t variant_ = DEFAULT_)
      : class_enum(class_), type(type_), version(version_), variant(variant_) {}

  EntityClassifier(const uint8_t class_, const uint8_t type_, const Version version_,
                   const uint8_t variant_ = DEFAULT_)
      : class_(class_), type(type_), version(version_), variant(variant_) {}

  EntityClassifier(const EntityClass class_, const uint8_t type_, const uint8_t version_major_,
                   const uint8_t version_minor_, const uint8_t version_patch_,
                   const uint8_t variant_ = DEFAULT_)
      : class_enum(class_), type(type_), version(version_major_, version_minor_, version_patch_),
        variant(variant_) {}

  EntityClassifier(const uint8_t class_, const uint8_t type_, const uint8_t version_major_,
                   const uint8_t version_minor_, const uint8_t version_patch_,
                   const uint8_t variant_ = DEFAULT_)
      : class_(class_), type(type_), version(version_major_, version_minor_, version_patch_),
        variant(variant_) {}

  template <class EntityType_> EntityType_ type_as() const { return static_cast<EntityType_>(type); }

  template <class EntityVariant_> EntityVariant_ variant_as() const {
    return static_cast<EntityVariant_>(variant);
  }

  template <class Version_> Version_ version_as() const { return static_cast<Version_>(version); }

  std::string to_string() const {
    return "(" + std::to_string(class_) + ", " + std::to_string(type) + ", " + version.to_string() + ", " +
           std::to_string(variant) + ")";
  }

  explicit operator bool() const {
    // All fields of a valid EntityClassifier must not be zero.
    return class_ and type and version and variant;
  }
};

static_assert(sizeof(EntityClassifier) == 6, "EntityClassifier has unexpected number of bytes.");

class Entity {
protected:
  std::string entity_id;

public:
  Entity() = default;

  explicit Entity(std::string entityId) : entity_id(std::move(entityId)) {}

  const std::string &get_entity_id() const { return entity_id; }

  virtual EntityClassifier get_entity_classifier() const {
    return {get_entity_class(), get_entity_type(), get_entity_version(), get_entity_variant()};
  }

  virtual EntityClass get_entity_class() const = 0;

  virtual uint8_t get_entity_type() const {
    // For many entities, there will only be one default type.
    return EntityClassifier::DEFAULT_;
  }

  virtual Version get_entity_version() const {
    // TODO: Make this pure virtual and read this from the HAL instances.
    return EntityClassifier::DEFAULT_VERSION_;
  }

  virtual uint8_t get_entity_variant() const {
    // For many entities, there will only be one default variation.
    return EntityClassifier::DEFAULT_;
  }

  bool is_entity_class(EntityClass class_) { return get_entity_class() == class_; }

  bool is_entity_type(uint8_t type_) { return get_entity_type() == type_; }

  bool is_entity_version(Version version_) { return get_entity_version() == version_; }

  bool is_entity_variant(uint8_t variant_) { return get_entity_variant() == variant_; }

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

  /**
   * Deserialize a new configuration for this entity and all its children from a JsonObject.
   * @returns true in case of success, else false
   **/
  utils::status config_from_json(JsonObjectConst cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
    Serial.println(__PRETTY_FUNCTION__);
#endif
    if (cfg.isNull())
      return utils::status("Configuration is Null at entity %s", get_entity_id().c_str());
    auto res = config_self_from_json(cfg);
    if(!res)
      return res;
    res = config_children_from_json(cfg);
    if(!res)
      return res;
    return utils::status::success();
  }

  /**
   * Serialize the configuration for this entity to a JsonObject.
   * @arg recursive If given, includes self configuration and all children.
   **/
  void config_to_json(JsonObject &cfg, bool recursive = true) {
    config_self_to_json(cfg);
    if (recursive)
      config_children_to_json(cfg);
  }

protected:
  /**
   * Deserialize a new configuration for this entity from a JsonObject.
   * Implementations shall not traverse to children, @see config_children_from_json() instead.
   * @returns true in case of success, else false
   **/
  virtual utils::status config_self_from_json(JsonObjectConst cfg) = 0;

  /**
   * Deserialize a new configuration for all child entities from a JsonObject.
   * Does not include own configuration, @see config_self_from_json() instead.
   * @returns true in case of success, else false
   **/
  utils::status config_children_from_json(JsonObjectConst &cfg) {
    for (JsonPairConst keyval : cfg) {
      if (keyval.key().c_str()[0] == '/' and keyval.key().size() > 1) {
        std::string child_id(keyval.key().c_str() + 1);
        auto child_entity = get_child_entity(child_id);
        if (!child_entity)
          return utils::status("Child entity '%s' does not exist at entity '%s'", child_id, get_entity_id().c_str());
        auto res = child_entity->config_from_json(keyval.value());
        if(!res) return res;
      }
    }
    return utils::status::success();
  }

  /**
   * Serialize the configuration of this entity to a JsonObject.
   * Implementations shall not traverse to children, @see config_children_to_json() instead.
   **/
  virtual void config_self_to_json(JsonObject &cfg) {
#ifdef ANABRID_DEBUG_ENTITY_CONFIG
    Serial.println(__PRETTY_FUNCTION__);
#endif
  }

  /**
   * Serialize the configuration of the child entities of this entity to a JsonObject.
   * Does not include own configuration, @see config_self_to_json() instead.
   **/
  void config_children_to_json(JsonObject &cfg) {
    for (auto child : get_child_entities()) {
      if (child) {
        auto child_cfg = cfg.createNestedObject(std::string("/") + child->get_entity_id());
        child->config_to_json(child_cfg, true);
      }
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

    auto versions_arr = dst.createNestedArray("version");
    versions_arr.add(src.version.major);
    versions_arr.add(src.version.minor);
    versions_arr.add(src.version.patch);
    return true;
  }

  static entities::EntityClassifier fromJson(JsonVariantConst src) {
    return {src["class"].as<uint8_t>(), src["type"],       src["version"][0],
            src["version"][1],          src["version"][2], src["variant"]};
  }

  static bool checkJson(JsonVariantConst src) {
    return src["class"].is<uint8_t>() and src["type"].is<uint8_t>() and src["variant"].is<uint8_t>() and
           src["version"].is<JsonArrayConst>() and src["version"].as<JsonArrayConst>().size() == 3;
  }
};

} // namespace ArduinoJson
