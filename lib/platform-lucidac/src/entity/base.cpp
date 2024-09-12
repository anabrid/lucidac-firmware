#include "entity/base.h"
#include "utils/mac.h" // toString(eui64)

FLASHMEM std::string entities::EntityClassifier::to_string() const {
    return "(" + std::to_string(class_) + ", " + std::to_string(type) + ", " + version.to_string() + ", " +
        std::to_string(variant) + ")";
}

FLASHMEM 
entities::EntityClassifier entities::Entity::get_entity_classifier() const {
  return {get_entity_class(), get_entity_type(), get_entity_version(), get_entity_variant()};
}

FLASHMEM
entities::Entity* entities::Entity::resolve_child_entity(std::string paths[], size_t len) {
    auto resolved_entity = this;
    for (size_t path_depth = 0; path_depth < len; path_depth++) {
      resolved_entity = resolved_entity->get_child_entity(paths[path_depth]);
      if (!resolved_entity) {
        return nullptr;
      }
    }
    return resolved_entity;
}

FLASHMEM
entities::Entity *entities::Entity::resolve_child_entity(JsonArrayConstIterator begin, JsonArrayConstIterator end) {
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

FLASHMEM
utils::status entities::Entity::config_from_json(JsonObjectConst cfg) {
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

FLASHMEM
utils::status entities::Entity::config_children_from_json(JsonObjectConst &cfg) {
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

FLASHMEM
void entities::Entity::config_children_to_json(JsonObject &cfg) {
    for (const auto& child : get_child_entities()) {
      if (child) {
        auto child_cfg = cfg.createNestedObject(std::string("/") + child->get_entity_id());
        child->config_to_json(child_cfg, true);
      }
    }
  }

FLASHMEM
bool ArduinoJson::Converter<entities::EntityClassifier>::toJson(const entities::EntityClassifier &src, JsonVariant dst) {
    dst["class"] = src.class_;
    dst["type"] = src.type;
    dst["variant"] = src.variant;

    auto versions_arr = dst.createNestedArray("version");
    versions_arr.add(src.version.major);
    versions_arr.add(src.version.minor);
    versions_arr.add(src.version.patch);
    return true;
}

FLASHMEM
entities::EntityClassifier ArduinoJson::Converter<entities::EntityClassifier>::fromJson(JsonVariantConst src) {
    return {src["class"].as<uint8_t>(), src["type"],       src["version"][0],
            src["version"][1],          src["version"][2], src["variant"]};
}

FLASHMEM
bool Converter<entities::EntityClassifier>::checkJson(JsonVariantConst src) {
    return src["class"].is<uint8_t>() and src["type"].is<uint8_t>() and src["variant"].is<uint8_t>() and
           src["version"].is<JsonArrayConst>() and src["version"].as<JsonArrayConst>().size() == 3;
}

FLASHMEM
void entities::Entity::classifier_to_json(JsonObject& out) {
  auto jsonvar = out["/" + get_entity_id()] = get_entity_classifier();
  auto jsonobj = jsonvar.as<JsonObject>();
  jsonobj["eui"] = utils::toString(get_entity_eui());

  for(auto& child_ptr : get_child_entities()) {
    if(child_ptr)
      child_ptr->classifier_to_json(jsonobj);
  }
}

FLASHMEM utils::status entities::Entity::user_set_config(JsonObjectConst msg_in, JsonObject &msg_out) {
#ifdef ANABRID_DEBUG_COMMS
  Serial.println(__PRETTY_FUNCTION__);
#endif
  auto self_entity_id = get_entity_id();
  if (!msg_in.containsKey("entity") or !msg_in.containsKey("config")) {
    return utils::status(1, "Malformed message.");
  }

  // Convert JSON array of possible anything to string array
  auto path_json = msg_in["entity"].as<JsonArrayConst>();
  auto path_depth = path_json.size();
  std::string path[path_depth];
  copyArray(path_json, path, path_depth);

  // Sanity check path, which must at least be addressed to us
  if (!path_depth) {
    return utils::status(2, "Invalid entity path (depth)");
  }
  if (path[0] != self_entity_id) {
    return utils::status(3, "Message intended for another entity (%s but I am %s)",
       path[0].c_str(), self_entity_id.c_str());
  }

  // Path may be to one of our sub-entities
  auto resolved_entity = resolve_child_entity(path + 1, path_depth - 1);
  if (!resolved_entity) {
    return utils::status(4, "Could not resolve child entity in given path");
  }

  utils::status res = resolved_entity->config_from_json(msg_in["config"]);
  if (!res) {
    // Could enrich with "could not apply configuration..."
    return res;
  }

  // Actually write to hardware
  utils::status hw_res = write_to_hardware();
  if(!hw_res) {
    return hw_res;
  }

  return utils::status::success();
}

FLASHMEM utils::status entities::Entity::user_get_config(JsonObjectConst msg_in, JsonObject &msg_out) {
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
      return utils::status(1, "Requested entity %s but I am %s", path[0].as<const char*>(), get_entity_id().c_str());
    } else {
      auto path_begin = path.begin();
      ++path_begin;
      entity = resolve_child_entity(path_begin, path.end());
      if (!entity) {
        return utils::status(2, "Cannot resolve entity path");
      }
    }
  } else {
    return utils::status(3, "Entity path is not a list");
  }

  // Save entity path back into response
  msg_out["entity"] = msg_in["entity"];
  // Save config into response
  auto cfg = msg_out.createNestedObject("config");
  entity->config_to_json(cfg, recursive);
  return utils::status::success();
}

FLASHMEM utils::status entities::Entity::user_reset_config(JsonObjectConst msg_in, JsonObject &msg_out) {
  entities::ResetAction reset_request(0);

  // msg_in["foo"] | default_value
  //    is a short way to write
  // msg_in.contains("foo") ? msg_in["foo"].as<bool>() : default_value

  bool keep_calibration = msg_in["keep_calibration"] | true;
  
  if(!keep_calibration)
    reset_request.val |= entities::ResetAction::CALIBRATION_RESET;

  bool overload_reset = msg_in["overload_reset"] | true;

  if(overload_reset)
    reset_request.val |= entities::ResetAction::OVERLOAD_RESET;
    // attention: Overload request always immediately writes to hardware
  
  bool circuit_reset = msg_in["circuit_reset"] | true;

  if(circuit_reset)
    reset_request.val |= entities::ResetAction::CIRCUIT_RESET;

  reset(reset_request);

  if (msg_in["sync"] | true) {
    auto status = write_to_hardware();
    if(!status) {
      return status;
    }
  }
  return utils::status::success();
}
