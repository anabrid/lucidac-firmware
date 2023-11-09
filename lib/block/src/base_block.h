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

#include "entity.h"
#include "local_bus.h"

namespace blocks {

/**
 * A function block represents one module in the LUCIDAC,
 * such as an M-Block, C-Block, I-Block or U-Block.
 *
 * This class (or its children) typically holds the in-RAM representation
 * of the configuration space of the particular hardware and serves as the
 * primary programming interface.
 *
 */
class FunctionBlock : public entities::Entity {
public:
  const uint8_t cluster_idx;

  explicit FunctionBlock(std::string entity_id, const uint8_t clusterIdx)
      : entities::Entity(std::move(entity_id)), cluster_idx(clusterIdx) {}

  virtual bool init() { return true; }

  virtual void reset(bool keep_calibration) {}

  virtual bus::addr_t get_block_address() = 0;

  virtual void write_to_hardware() = 0;

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