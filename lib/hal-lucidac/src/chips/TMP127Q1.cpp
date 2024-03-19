// Copyright (c) 2024 anabrid GmbH
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

#include "TMP127Q1.h"

const SPISettings functions::TMP127Q1::DEFAULT_SPI_SETTINGS{4'000'000, MSBFIRST, SPI_MODE0};

float functions::TMP127Q1::read_temperature() const {
  auto raw = transfer16(0);
  return raw_to_float(raw);
}

float functions::TMP127Q1::raw_to_float(uint16_t raw) {
  auto signed_raw = raw_to_signed_raw(raw);
  return static_cast<float>(signed_raw) * 0.03125f;
}

int16_t functions::TMP127Q1::raw_to_signed_raw(uint16_t raw) {
  // Convert to signed integer and use arithmetic right-shift, which keeps sign bit.
  // Last two bits are always 1 and need to be shifted out to result in 14bit value.
  auto signed_raw = static_cast<int16_t>(raw);
  return static_cast<int16_t>(signed_raw >> 2);
}

functions::TMP127Q1::TMP127Q1(unsigned short address) : DataFunction(address, DEFAULT_SPI_SETTINGS) {}
