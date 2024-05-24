// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "cblock.h"
#include "iblock.h"
#include "mblock.h"
#include "ublock.h"

#include "logging.h"
#include "metadata/metadata.h"

namespace blocks {

template <class BlockT> BlockT *detect(const bus::addr_t block_address) {
  metadata::MetadataReader reader(block_address);
  auto classifier = reader.read_entity_classifier();
  LOG(ANABRID_DEBUG_INIT, (std::string("Read classifier ") + classifier.to_string() + " at address " +
                           std::to_string(block_address) + ".")
                              .c_str());

  switch (classifier.class_enum) {
  case entities::EntityClass::UNKNOWN:
  case entities::EntityClass::CARRIER:
  case entities::EntityClass::CLUSTER:
    return nullptr;
  case entities::EntityClass::M_BLOCK:
  case entities::EntityClass::U_BLOCK:
  case entities::EntityClass::C_BLOCK:
  case entities::EntityClass::I_BLOCK:
    return BlockT::from_entity_classifier(classifier, block_address);
  }
}

} // namespace blocks
