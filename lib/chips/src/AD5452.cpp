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

#include "AD5452.h"

const SPISettings functions::AD5452::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE1};

functions::AD5452::AD5452(bus::addr_t address) : DataFunction(address, DEFAULT_SPI_SETTINGS) {}

functions::AD5452::AD5452(bus::addr_t base_addr, uint8_t func_addr_shift)
    : AD5452(bus::increase_function_idx(base_addr, func_addr_shift)) {}

uint16_t functions::AD5452::float_to_raw(float scale) {
  // Scaling between +-2 and +-20 coefficient happens outside!
  if (scale <= -2.0f)
    return 0;
  if (scale >= +2.0f)
    return 4095 << 2;
  return static_cast<uint16_t>(scale * 1024.0f + 2047) << 2;
}

void functions::AD5452::set_scale(uint16_t scale_raw) {
  begin_communication();
  // AD5452 expects at least 13ns delay between chip select and data
  delayNanoseconds(15);
  bus::spi.transfer16(scale_raw);
  end_communication();
}

void functions::AD5452::set_scale(float scale) {
  // Scaling between +-1 and +-10 coefficient happens outside!
  return set_scale(float_to_raw(scale));
}
