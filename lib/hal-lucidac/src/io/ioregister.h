// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <cstdint>

namespace io {

void change_register(volatile uint32_t &reg, uint32_t clear_mask, uint32_t set_mask);

} // namespace io
