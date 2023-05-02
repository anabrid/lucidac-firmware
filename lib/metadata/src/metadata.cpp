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

#include "metadata.h"

metadata::MetadataMemory74LVC138AD::MetadataMemory74LVC138AD(const local_bus::addr_t address) : address(address) {}

size_t metadata::MetadataMemory74LVC138AD::read_from_hardware(size_t byte_offset, size_t length, uint8_t *buffer) const {
    // Begin SPI transaction and address self
    const SPISettings spi_cfg{4'000'000, MSBFIRST, SPI_MODE0};
    SPI1.beginTransaction(spi_cfg);
    local_bus::address_function(address);

    // Send READ command B00000011
    SPI1.transfer(0x3);
    // Send byte offset
    SPI1.transfer(byte_offset);
    // Read data
    SPI1.transfer(nullptr, buffer, length);

    // Release address and stop SPI transaction
    local_bus::release_address();
    SPI1.endTransaction();

    return length;
}

size_t metadata::MetadataMemory74LVC138AD::read_from_hardware() {
    return read_from_hardware(0, sizeof(data), data);
}
