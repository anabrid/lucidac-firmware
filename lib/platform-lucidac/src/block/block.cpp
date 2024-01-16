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

#include "block.h"
#include "bus/bus.h"
#include "bus/functions.h"

void blocks::CScaleSwitchFunction::write_to_hardware() const {
  begin_communication();
  bus::spi.transfer32(data);
  end_communication();
}

blocks::CScaleSwitchFunction::CScaleSwitchFunction(bus::addr_t address)
    : _old_DataFunction(address, SPISettings(4'000'000, MSBFIRST,
                                        SPI_MODE3 /* Chip expects MODE0, CLK is inverted on the way */)),
      data(0) {}

blocks::CCoeffFunction::CCoeffFunction(bus::addr_t base_address, uint8_t coeff_idx)
    : _old_DataFunction(bus::increase_function_idx(base_address, coeff_idx),
                   SPISettings(4'000'000, MSBFIRST, SPI_MODE1)),
      data{0} {}

void blocks::CCoeffFunction::write_to_hardware() const {
  begin_communication();
  // AD5452 expects at least 13ns delay between chip select and data
  delayNanoseconds(15);
  bus::spi.transfer16(data);
  end_communication();
}
