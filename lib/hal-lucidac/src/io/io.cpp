// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>

#include "io.h"

void io::init() { pinMode(PIN_BUTTON, INPUT_PULLUP); }

bool io::get_button() { return !digitalReadFast(PIN_BUTTON); }

void io::block_until_button_press() {
  while (!get_button()) {
  }
}
