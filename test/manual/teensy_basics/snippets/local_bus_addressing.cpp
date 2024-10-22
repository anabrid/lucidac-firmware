// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "bus/bus.h"
#include <Arduino.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  bus::init();
}

void loop() {
  bus::address_block(0, 5);
  digitalWriteFast(LED_BUILTIN, HIGH);
  for (uint32_t i = 0; i < 64; i++) {
    bus::address_function(0, 5, i);
  }
  digitalWriteFast(LED_BUILTIN, LOW);
  bus::address_block(0, 0);
  delayMicroseconds(200);
}
