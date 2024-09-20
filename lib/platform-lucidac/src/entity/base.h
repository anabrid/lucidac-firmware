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

#include "utils/error.h"
#include "version.h"

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

  std::string to_string() const;

  explicit operator bool() const {
    // All fields of a valid EntityClassifier must not be zero.
    return class_ and type and version and variant;
  }
};

static_assert(sizeof(EntityClassifier) == 6, "EntityClassifier has unexpected number of bytes.");

struct ResetAction {
  constexpr static uint8_t CIRCUIT_RESET = 1 << 1;
  constexpr static uint8_t CALIBRATION_RESET = 1 << 2;
  constexpr static uint8_t OVERLOAD_RESET = 1 << 3;
  constexpr static uint8_t EVERYTHING = 0xFF;

  uint8_t val;

  ResetAction(uint8_t val) : val(val) {}

  inline bool has(uint8_t other) { return other & val; }
};

class Entity {
protected:
  std::string entity_id;

public:
  Entity() = default;

  explicit Entity(std::string entityId) : entity_id(std::move(entityId)) {}

  const std::string &get_entity_id() const { return entity_id; }

  virtual EntityClassifier get_entity_classifier() const;

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

  virtual std::array<uint8_t, 8> get_entity_eui() const = 0;

  bool is_entity_class(EntityClass class_) { return get_entity_class() == class_; }

  bool is_entity_type(uint8_t type_) { return get_entity_type() == type_; }

  bool is_entity_version(Version version_) { return get_entity_version() == version_; }

  bool is_entity_variant(uint8_t variant_) { return get_entity_variant() == variant_; }

  virtual std::vector<Entity *> get_child_entities() = 0;

  virtual Entity *get_child_entity(const std::string &child_id) = 0;

  Entity *resolve_child_entity(std::string paths[], size_t len);

  Entity *resolve_child_entity(JsonArrayConstIterator begin, JsonArrayConstIterator end);

  Entity *resolve_child_entity(JsonArrayConst path) { return resolve_child_entity(path.begin(), path.end()); }

  /// returns true in case of success
  virtual bool init() { return true; }

  virtual void reset(ResetAction action) {}

  /// returns true in case of success
  [[nodiscard]] virtual utils::status write_to_hardware() { return utils::status::success(); }

  /**
   * Deserialize a new configuration for this entity and all its children from a JsonObject.
   * @returns true in case of success, else false
   **/
  utils::status config_from_json(JsonObjectConst cfg);

  /**
   * Serialize the configuration for this entity to a JsonObject.
   * @arg recursive If given, includes self configuration and all children.
   **/
  void config_to_json(JsonObject &cfg, bool recursive = true) {
    config_self_to_json(cfg);
    if (recursive)
      config_children_to_json(cfg);
  }

  ///@addtogroup User-Functions
  ///@{
  utils::status user_set_config(JsonObjectConst msg_in, JsonObject &msg_out);
  utils::status user_get_config(JsonObjectConst msg_in, JsonObject &msg_out);
  utils::status user_reset_config(JsonObjectConst msg_in, JsonObject &msg_out);
  ///@}

  /** Provide recursive entity information in a tree */
  void classifier_to_json(JsonObject &out);

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
  utils::status config_children_from_json(JsonObjectConst &cfg);

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
  void config_children_to_json(JsonObject &cfg);
};

} // namespace entities

namespace ArduinoJson {

template <> struct Converter<entities::EntityClassifier> {
  static bool toJson(const entities::EntityClassifier &src, JsonVariant dst);
  static entities::EntityClassifier fromJson(JsonVariantConst src);
  static bool checkJson(JsonVariantConst src);
};

} // namespace ArduinoJson

// As this uses static variables, we only allow using this in test cases

#ifdef PIO_UNIT_TESTING

#include <ostream>

inline std::ostream &operator<<(std::ostream &os, entities::Entity &entity) {
  static StaticJsonDocument<2048> doc;
  doc.clear();
  static JsonObject cfg = doc.to<JsonObject>();
  cfg.clear();

  entity.config_to_json(cfg, true);

  os << cfg;

  return os;
}

#endif
