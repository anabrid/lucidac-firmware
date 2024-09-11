// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>

#include "entity/base.h"

#include "bus/bus.h"
#include "metadata/metadata.h"
#include "utils/logging.h"

namespace metadata {

/**
 * This structure describes the 256 byte memory of the 25AA02E64 EEPROMs in Memory Version 1.
 **/

enum class LayoutVersion : uint8_t { V1 = 1 };

struct __attribute__((packed)) MetadataMemoryLayoutV1 {
  const LayoutVersion version;
  const uint16_t size;
  const entities::EntityClassifier classifier;
  const uint8_t entity_data[256 - 17];
  const uint8_t uuid[8];
};

class MetadataEditor : public functions::EEPROM25AA02 {
public:
  using functions::EEPROM25AA02::EEPROM25AA02;

  entities::EntityClassifier read_entity_classifier() {
    std::array<uint8_t, sizeof(entities::EntityClassifier)> data{0};
    EEPROM25AA02::read(offsetof(MetadataMemoryLayoutV1, classifier), data.size(), data.data());
    return {data[0], data[1], data[2], data[3], data[4], data[5]};
  }

  std::array<uint8_t, 8> read_eui() {
    std::array<uint8_t, 8> data{0};
    EEPROM25AA02::read(offsetof(MetadataMemoryLayoutV1, uuid), data.size(), data.data());
    return data;
  }

  bool write_entity_classifier(const entities::EntityClassifier &classifier) {
    return EEPROM25AA02::write(offsetof(MetadataMemoryLayoutV1, classifier), sizeof(classifier),
                               reinterpret_cast<const uint8_t *>(std::addressof(classifier)));
  }

  size_t read(size_t address, size_t length, uint8_t *buffer) const = delete;
  bool read8(size_t address, uint8_t *data_out) const = delete;
  bool read16(size_t address, uint16_t *data_out) const = delete;
  bool read32(size_t address, uint32_t *data_out) const = delete;
  size_t write(size_t address, size_t length, const uint8_t *buffer) const = delete;
  bool write8(size_t address, uint8_t data) const = delete;
  bool write16(size_t address, uint16_t data) const = delete;
  bool write32(size_t address, uint32_t data) const = delete;
};
} // namespace metadata

namespace entities {

template <class BlockT> BlockT *detect(const bus::addr_t block_address) {
  LOG(ANABRID_DEBUG_INIT, __PRETTY_FUNCTION__);
  metadata::MetadataEditor reader(block_address);
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
