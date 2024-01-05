// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "local_bus/local_bus.h"
#include "metadata/metadata.h"

/**
 * The 74LVC138AD is a 3-Line to 8-Line decoder/Demultiplexer.
 * Actually this class provides access to the 25AA02E64 2K SPI Bus Serial EEPROM
 * with EUI-64 Node Identity, holding 2048 bits = 256 bytes of meta data and identification
 * material (see also Deliverable D3.2 section 3.1). This is represented by the
 * the MetadataMemory<256> base class.
 **/
class MetadataMemory74LVC138AD : public metadata::MetadataMemory<256> {
public:
  explicit MetadataMemory74LVC138AD(bus::addr_t address)
      : metadata::MetadataMemory<256>(address, SPISettings(4'000'000, MSBFIRST, SPI_MODE0)) {}

  using MetadataMemory<256>::read_from_hardware;

  size_t read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const override {
    // Begin SPI communication
    begin_communication();

    // Send READ command B00000011
    bus::spi.transfer(0x3);
    // Send byte offset
    bus::spi.transfer(byte_offset);
    // Read data
    bus::spi.transfer(nullptr, buffer, length);

    // Release address and stop SPI transaction
    end_communication();

    return length;
  }
};