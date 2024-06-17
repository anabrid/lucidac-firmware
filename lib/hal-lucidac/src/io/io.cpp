// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>

#include "io.h"

void io::init() {
  for (auto pin :
       {PIN_BUTTON, PIN_DIO_6, PIN_DIO_11, PIN_DIO_12, PIN_DIO_13, PIN_DIO_23, PIN_DIO_28, PIN_RESERVED_7})
    pinMode(pin, INPUT_PULLUP);
}

bool io::get_button() { return !digitalReadFast(PIN_BUTTON); }

void io::block_until_button_press() {
  while (!get_button()) {
  }
}
