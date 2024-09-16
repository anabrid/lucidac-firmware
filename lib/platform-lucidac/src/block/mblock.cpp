// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "block/mblock.h"
#include "utils/logging.h"

#include "carrier/cluster.h"

#include "io/io.h" // Just for testing TODO REMOVE

FLASHMEM blocks::MBlock::MBlock(bus::addr_t block_address, MBlockHAL* hardware)
    : blocks::FunctionBlock{std::string("M") + std::string(
                                                   // Addresses 12, 20, 28 are M0
                                                   // Addresses 13, 21, 29 are M1
                                                   block_address % 8 == 4 ? "0" : "1"),
                            block_address},
      slot(block_address % 8 == 4 ? SLOT::M0 : SLOT::M1),
      hardware(hardware) {}

FLASHMEM uint8_t blocks::MBlock::slot_to_global_io_index(uint8_t local) const {
  switch (slot) {
  case SLOT::M0:
    return local;
  case SLOT::M1:
    return local + 8;
  }
  // This should never be reached
  return local;
}

FLASHMEM 
blocks::MBlock *blocks::MBlock::from_entity_classifier(entities::EntityClassifier classifier,
                                                       const bus::addr_t block_address) {
  if (!classifier or classifier.class_enum != entities::EntityClass::M_BLOCK)
    return nullptr;

  auto type = classifier.type_as<TYPES>();
  switch (type) {
  case TYPES::UNKNOWN:
    // This is already checked by !classifier above
    return nullptr;
  case TYPES::M_MUL4_BLOCK:
    return MMulBlock::from_entity_classifier(classifier, block_address);
  case TYPES::M_INT8_BLOCK:
    return MIntBlock::from_entity_classifier(classifier, block_address);
  }
  // Any unknown value results in a nullptr here.
  // Adding default case to switch suppresses warnings about missing cases.
  return nullptr;
}

FLASHMEM utils::status blocks::EmptyMBlock::write_to_hardware() { return utils::status::success(); }

FLASHMEM utils::status blocks::EmptyMBlock::config_self_from_json(JsonObjectConst cfg) {
  return utils::status::success();
}

FLASHMEM uint8_t blocks::EmptyMBlock::get_entity_type() const { return static_cast<uint8_t>(MBlock::TYPES::UNKNOWN); }

bool blocks::MBlockHAL::init() {
  if (!FunctionBlockHAL::init())
    return false;
  reset_overload_flags();
  return true;
}
