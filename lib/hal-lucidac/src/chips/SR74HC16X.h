// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "bus/functions.h"

namespace functions {

class SR74HC16X : public DataFunction {
public:
  static const SPISettings DEFAULT_SPI_SETTINGS;

  explicit SR74HC16X(bus::addr_t address);

  uint8_t read8() const;
};

}