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

#include <array>
#include <cstdint>

#include "local_bus.h"

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

template <std::size_t dataSize> class MetadataMemory : public bus::DataFunction {
private:
  std::array<uint8_t, dataSize> data;

public:
  explicit MetadataMemory(const unsigned short address, const SPISettings spi_settings)
      : bus::DataFunction(address, spi_settings), data{0} {}

  virtual size_t read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const = 0;
  size_t read_from_hardware() { return read_from_hardware(0, data.size(), data.data()); };
};

} // namespace metadata