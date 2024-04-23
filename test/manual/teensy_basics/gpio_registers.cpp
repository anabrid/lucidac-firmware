// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>

void setup() {
  // This can also be replaced by register manipulation, but there is no real reason.
  for (auto pin : {23, 22, 21, 20, 19, 18, 17, 16})
    pinMode(pin, OUTPUT);

  // For more Pin information check Teensy documentation, primarily
  // https://www.pjrc.com/teensy/schematic.html
  // https://github.com/KurtE/TeensyDocuments/
}

void loop() {
  delay(100);
  static bool value = true;
  value = !value;

  // for(auto pin: {23, 22, 21, 20, 19, 18, 17, 16})
  //     digitalWriteFast(pin, value);

  // See .platformio/packages/framework-arduinoteensy/cores/teensy4/core_pins.h
  // Or just check what `digitalWriteFast` does
  auto mask = CORE_PIN23_BITMASK | CORE_PIN22_BITMASK | CORE_PIN21_BITMASK | CORE_PIN20_BITMASK |
              CORE_PIN19_BITMASK | CORE_PIN18_BITMASK | CORE_PIN17_BITMASK | CORE_PIN16_BITMASK;
  if (value) {
    GPIO6_DR_SET = mask;
    // This does not need |= apparently
    // CORE_PIN23_PORTSET = CORE_PIN23_BITMASK;
  } else {
    GPIO6_DR_CLEAR = mask;
    // CORE_PIN23_PORTCLEAR = CORE_PIN23_BITMASK;
  }
}
