// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "bus/bus.h"
#include "bus/functions.h"

namespace metadata {

typedef struct __attribute__((packed)) MetadataMemoryLayoutV1 {
  const uint16_t _memory_version_and_size;
  const uint16_t _entity_class_and_type;
  const uint8_t entity_variant;
  const uint8_t entity_version;
  const uint8_t entity_data[256 - 14];
  const uint8_t uuid[8];

  uint8_t get_memory_version() const { return (_memory_version_and_size & 0xE000) >> 13; }

  uint16_t get_memory_size() const { return _memory_version_and_size & 0x1FFF; }
} MetadataMemoryLayoutV1;

template <std::size_t dataSize> class MetadataMemory : public functions::DataFunction {
private:
  std::array<uint8_t, dataSize> data;

public:
  explicit MetadataMemory(const unsigned short address, const SPISettings &spi_settings)
      : functions::DataFunction(address, spi_settings), data{0} {}

  virtual size_t read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const = 0;

  size_t read_from_hardware() { return read_from_hardware(0, data.size(), data.data()); };
};

} // namespace metadata