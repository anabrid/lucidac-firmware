// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "bus/functions.h"
#include "bus/bus.h"

namespace metadata {

/**
 * This structure describes the 256 byte memory of the 25AA02E64 EEPROMs in Memory Version 1.
 **/
typedef struct __attribute__((packed)) MetadataMemoryLayoutV1 {
  const uint16_t _memory_version_and_size;
  const uint16_t _entity_class_and_type;
  const uint8_t entity_variant;
  const uint8_t entity_version;
  const uint8_t entity_data[256 - 14];
  const uint8_t eui64[8];

  uint8_t get_memory_version() const { return (_memory_version_and_size & 0xE000) >> 13; }

  uint16_t get_memory_size() const { return _memory_version_and_size & 0x1FFF; }
} MetadataMemoryLayoutV1;

/**
 * The abstract metadata memory class allows to represent different EEPROM sizes, for instance
 * does the 25AA02E64 hold 256 bytes while 25AA02E32 holds only 128 bytes. However, the extension
 * rather looks into bigger then smaller EEPROMs.
 **/
template <std::size_t dataSize> class MetadataMemory : public functions::DataFunction {
private:
  std::array<uint8_t, dataSize> data;

public:
  explicit MetadataMemory(const unsigned short address, const SPISettings& spi_settings)
      : functions::DataFunction(address, spi_settings), data{0} {}

  virtual size_t read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const = 0;
  size_t read_from_hardware() { return read_from_hardware(0, data.size(), data.data()); };
};


} // namespace metadata