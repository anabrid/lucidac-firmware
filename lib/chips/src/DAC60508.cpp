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

#include "DAC60508.h"

const SPISettings functions::DAC60508::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE1};

uint16_t functions::DAC60508::read_register(uint8_t address) const {
  begin_communication();
  get_raw_spi().transfer((address & 0b0'000'1111) | 0b1'000'0000);
  get_raw_spi().transfer16(0);
  end_communication();
  begin_communication();
  get_raw_spi().transfer(0);
  auto data = get_raw_spi().transfer16(0);
  end_communication();
  return data;
}

bool functions::DAC60508::write_register(uint8_t address, uint16_t data) const {
  begin_communication();
  get_raw_spi().transfer(address & 0b0'000'1111);
  get_raw_spi().transfer16(data);
  end_communication();
  // One could possibly read the register back to check or use CRC
  return true;
}

functions::DAC60508::DAC60508(bus::addr_t address) : bus::DataFunction(address, DEFAULT_SPI_SETTINGS) {}

uint16_t functions::DAC60508::float_to_raw(float value) {
  // TODO: Potentially, each channel must be calibrated.
  if (value <= -1.0f)
    return RAW_MINUS_ONE;
  if (value >= +1.0f)
    return RAW_PLUS_ONE;
  return static_cast<uint16_t>((value + 1.0f) / 2.0f * static_cast<float>(RAW_PLUS_ONE - RAW_MINUS_ONE));
}

bool functions::DAC60508::set_channel(uint8_t idx, uint16_t value) const {
  write_register(REG_DAC(idx), value);
  // One could possibly read the register back to check or use CRC
  return true;
}

void functions::DAC60508::init() {
  // TODO: Change back to external reference when working again
  write_register(functions::DAC60508::REG_CONFIG, 0b0000'0000'0000'0000);
  write_register(functions::DAC60508::REG_GAIN, 0b0000'0000'1111'1111);
}
