// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/functions.h"

namespace functions {

class AD8402 : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  using DataFunction::DataFunction;
  //! Base resistance R_AB in kilo ohms is determined by the actual used chip. Possible values are 1, 10, 50,
  //! 100
  AD8402(bus::addr_t address, unsigned int base_resistance);

  //! Sets the resistance R_WB of chanel 1 to the according perecntage. A value of 1.0 coresponds to the
  //! maximum resistance R_AB
  void write_ch0_resistance(float percent);
  //! Sets the resistance R_WB of chanel 2 to the according perecntage. A value of 1.0 coresponds to the
  //! maximum resistance R_AB
  void write_ch1_resistance(float percent);

private:
  unsigned int _base_resistance;
};

} // namespace functions
