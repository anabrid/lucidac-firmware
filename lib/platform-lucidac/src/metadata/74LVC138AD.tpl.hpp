// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/bus.h"
#include "metadata.h"

class MetadataMemory74LVC138AD : public metadata::MetadataMemory<256> {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  explicit MetadataMemory74LVC138AD(bus::addr_t address)
      : metadata::MetadataMemory<256>(address, DEFAULT_SPI_SETTINGS) {}

  using MetadataMemory<256>::read_from_hardware;

  size_t read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const override {
    // Begin SPI communication
    begin_communication();

    auto spi = get_raw_spi();
    // Send READ command B00000011
    spi.transfer(0x3);
    // Send byte offset
    spi.transfer(byte_offset);
    // Read data
    spi.transfer(nullptr, buffer, length);

    // Release address and stop SPI transaction
    end_communication();

    return length;
  }
};

const SPISettings MetadataMemory74LVC138AD::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE0};