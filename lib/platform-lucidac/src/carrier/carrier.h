// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <array>
#include <string>

#include "chips/TMP127Q1.h"
#include "cluster.h"
#include "entity/entity.h"

using namespace platform;

namespace carrier {

/**
 * \brief Top-level hierarchy controlled by a single microcontroller
 * 
 * A Carrier (also refered to as module holder, base board or mother board) contains
 * one microcontroller and multiple clusters, where the clusers hold the actual analog
 * computing hardware.
 * 
 * \ingroup Singletons
 **/
class Carrier : public entities::Entity {
public:
  std::vector<Cluster> clusters;

  explicit Carrier(std::vector<Cluster> clusters);

  entities::EntityClass get_entity_class() const final;

  virtual bool init();

  std::vector<Entity *> get_child_entities() override;

  Entity *get_child_entity(const std::string &child_id) override;

  bool config_self_from_json(JsonObjectConst cfg) override;

  [[nodiscard]] virtual bool write_to_hardware();

  // REV1 specific things
  // TODO: These are partly LUCIDAC specific things, which should be rebased on `56-refactor-...` branch.
public:
  // Module addresses
  static constexpr uint8_t CARRIER_MADDR = 5;


  ///@addtogroup User-Functions
  ///@{
  int set_config(JsonObjectConst msg_in, JsonObject &msg_out);
  int get_config(JsonObjectConst msg_in, JsonObject &msg_out);
  int get_entities(JsonObjectConst msg_in, JsonObject &msg_out);
  int reset(JsonObjectConst msg_in, JsonObject &msg_out);
  ///@}
};

} // namespace carrier
