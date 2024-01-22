// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>

#include "carrier/cluster.h"
#include "entity/entity.h"

using namespace lucidac;

namespace carrier {

class Carrier : public entities::Entity {
public:
  std::array<LUCIDAC, 1> clusters;

  Carrier();

  bool init();

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

  void write_to_hardware();

  static Carrier& get();

  // functions exposed to end user
  int set_config(JsonObjectConst msg_in, JsonObject &msg_out);
  int get_config(JsonObjectConst msg_in, JsonObject &msg_out);
  int get_entities(JsonObjectConst msg_in, JsonObject &msg_out);
  int reset(JsonObjectConst msg_in, JsonObject &msg_out);
};

} // namespace carrier
