// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "entity/entity.h"
#include "bus/bus.h"

namespace blocks {

class FunctionBlockHAL {
  virtual float read_temperature() = 0;
};

/**
 * A function block represents one module in a cluster,
 * such as an M-Block, C-Block, I-Block or U-Block.
 *
 * This class (or its children) typically holds the in-RAM representation
 * of the configuration space of the particular hardware and serves as the
 * primary programming interface.
 *
 */
class FunctionBlock : public entities::Entity {
public:
  const bus::addr_t block_address;

  FunctionBlock(std::string entity_id, const bus::addr_t block_address)
      : entities::Entity(std::move(entity_id)), block_address(block_address) {}

  virtual bool init() { return true; }

  virtual void reset(bool keep_calibration) {}

  [[nodiscard]] virtual bool write_to_hardware() = 0;

  std::vector<Entity *> get_child_entities() override {
#ifdef ANABRID_DEBUG_ENTITY
    Serial.println(__PRETTY_FUNCTION__);
#endif
    // FunctionBlocks do not give direct access to their children
    return {};
  }

  Entity *get_child_entity(const std::string &child_id) override {
    // FunctionBlocks do not give direct access to their children
    return nullptr;
  }
};

} // namespace blocks
