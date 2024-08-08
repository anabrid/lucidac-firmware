// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "base.h"

#include "bus/bus.h"
#include "metadata/metadata.h"
#include "utils/logging.h"

namespace entities {

template <class BlockT> BlockT *detect(const bus::addr_t block_address) {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  metadata::MetadataReader reader(block_address);
  auto classifier = reader.read_entity_classifier();
  LOG(ANABRID_DEBUG_INIT, (std::string("Read classifier ") + classifier.to_string() + " at address " +
                           std::to_string(block_address) + ".")
                              .c_str());

  switch (classifier.class_enum) {
  case EntityClass::UNKNOWN:
  case EntityClass::CARRIER:
  case EntityClass::CLUSTER:
    return nullptr;
  case EntityClass::FRONT_PANEL:
  case EntityClass::M_BLOCK:
  case EntityClass::U_BLOCK:
  case EntityClass::C_BLOCK:
  case EntityClass::I_BLOCK:
  case EntityClass::SH_BLOCK:
  case EntityClass::CTRL_BLOCK:
    return BlockT::from_entity_classifier(classifier, block_address);
  }
  return nullptr;
}

} // namespace entities
