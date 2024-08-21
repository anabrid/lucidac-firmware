// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <array>
#include <cstdint>

#include "bus/bus.h"
#include "bus/functions.h"
#include "chips/EEPROM25AA02.h"
#include "entity/base.h"

namespace metadata {

/**
 * The abstract metadata memory class allows to represent different EEPROM sizes, for instance
 * does the 25AA02E64 hold 256 bytes while 25AA02E32 holds only 128 bytes. However, the extension
 * rather looks into bigger then smaller EEPROMs.
 **/
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
