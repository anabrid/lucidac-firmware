// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <cstdint>

#include "bus/functions.h"

namespace functions {

class TMP127Q1 : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  using DataFunction::DataFunction;
  explicit TMP127Q1(unsigned short address);

  static int16_t raw_to_signed_raw(uint16_t raw);
  static float raw_to_float(uint16_t raw);

  float read_temperature() const;
};

}

// FUNC_IDX is 33

