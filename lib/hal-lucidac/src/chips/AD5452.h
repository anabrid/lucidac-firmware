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

#include "bus/functions.h"

namespace functions {

class AD5452 : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;
  static constexpr uint16_t RAW_ZERO = 2047;

  using DataFunction::DataFunction;
  explicit AD5452(bus::addr_t address);
  AD5452(bus::addr_t base_addr, uint8_t func_addr_shift);

  void set_scale(uint16_t scale_raw) const;
  void set_scale(float scale) const;
  static uint16_t float_to_raw(float scale);
  static float raw_to_float(uint16_t raw);
};

}