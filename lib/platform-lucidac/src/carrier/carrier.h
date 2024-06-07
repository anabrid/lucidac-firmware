// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>

#include "cluster.h"
#include "chips/TMP127Q1.h"
#include "entity/entity.h"
#include "mac_utils.h"

using namespace platform;

namespace carrier {

class Carrier : public entities::Entity {
public:
  std::vector<Cluster> clusters;

  explicit Carrier(std::vector<Cluster> clusters);

  entities::EntityClass get_entity_class() const final;

  virtual bool init();

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

  void write_to_hardware();

  // REV1 specific things
  // TODO: These are partly LUCIDAC specific things, which should be rebased on `56-refactor-...` branch.
public:
  // Module addresses
  static constexpr uint8_t CARRIER_MADDR = 5;
};

} // namespace carrier
