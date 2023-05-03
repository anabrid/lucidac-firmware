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