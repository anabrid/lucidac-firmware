// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

namespace io {

constexpr uint8_t PIN_BUTTON = 29;
constexpr uint8_t PIN_DIO_6 = 31;
// constexpr uint8_t PIN_DIO_7 = 21; // CARE: Actually in use as EXTHALT on new flexio:3 mode
constexpr uint8_t PIN_DIO_11 = 0;
constexpr uint8_t PIN_DIO_12 = 38;
constexpr uint8_t PIN_DIO_13 = 13; // CARE: Is LED_BUILTIN
constexpr uint8_t PIN_DIO_23 = 1;
constexpr uint8_t PIN_DIO_28 = 33;
constexpr uint8_t PIN_RESERVED_7 = 30;

void init();

bool get_button();

void block_until_button_press_and_release();

} // namespace io