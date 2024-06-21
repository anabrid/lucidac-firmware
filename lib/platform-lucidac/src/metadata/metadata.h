// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "bus/bus.h"
#include "bus/functions.h"
#include "chips/EEPROM25AA02.h"
#include "entity/entity.h"

namespace metadata {

enum class LayoutVersion : uint8_t { V1 = 1 };

typedef struct __attribute__((packed)) MetadataMemoryLayoutV1 {
  const LayoutVersion version;
  const uint16_t size;
  const entities::EntityClassifier classifier;
  const uint8_t entity_data[256 - 15];
  const uint8_t uuid[8];
} MetadataMemoryLayoutV1;

class MetadataReader : public functions::EEPROM25AA02 {
public:
  using functions::EEPROM25AA02::EEPROM25AA02;

  entities::EntityClassifier read_entity_classifier() {
    std::array<uint8_t, 4> data{0};
    read(offsetof(MetadataMemoryLayoutV1, classifier), data.size(), data.data());
    return {data[0], data[1], data[2], data[3]};
  }
};

template <std::size_t dataSize> class MetadataMemory : public functions::DataFunction {
private:
  std::array<uint8_t, dataSize> data;

public:
  explicit MetadataMemory(const unsigned short address, const SPISettings &spi_settings)
      : functions::DataFunction(address, spi_settings), data{0} {}

  virtual size_t read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const = 0;

  size_t read_from_hardware() { return read_from_hardware(0, data.size(), data.data()); };

  template <class Layout_> const Layout_ &as() const { return *((Layout_ *)(data.data())); }
};

} // namespace metadata
