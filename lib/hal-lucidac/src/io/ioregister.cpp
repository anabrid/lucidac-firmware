// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "ioregister.h"

void io::change_register(volatile uint32_t &reg, uint32_t clear_mask, uint32_t set_mask) {
  reg = (reg & ~clear_mask) | set_mask;
}
