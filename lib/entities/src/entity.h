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

#include <string>

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
};

} // namespace entities