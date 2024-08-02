// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "shblock.h"

blocks::SHBlock::SHBlock(const bus::addr_t block_address) : FunctionBlock("SH", block_address) {}

blocks::SHBlock::SHBlock() : SHBlock(bus::BLOCK_BADDR(0, bus::SH_BLOCK_IDX)) {}

bool blocks::SHBlock::config_self_from_json(JsonObjectConst cfg) { return false; }

bool blocks::SHBlock::write_to_hardware() { return true; }

blocks::SHBlock *blocks::SHBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                         bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != CLASS_)
    return nullptr;

  // Currently, there are no different variants or versions
  if (classifier.variant != entities::EntityClassifier::DEFAULT_ or
      classifier.version != entities::EntityClassifier::DEFAULT_)
    return nullptr;

  // Return default implementation
  return new SHBlock(block_address);
}

void blocks::SHBlock::compensate_hardware_offsets() {
  set_track.trigger();
  delay(100);
  set_inject.trigger();
  delay(100);
}