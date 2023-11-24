// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "local_bus.h"
#include "metadata.h"

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