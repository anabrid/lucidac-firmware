// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

namespace io {

constexpr uint8_t PIN_BUTTON = 29;

void init();

bool get_button();

void block_until_button_press();

} // namespace io