// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// 
// ANABRID_BEGIN_LICENSE
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later
// ANABRID_END_LICENSE

/*
 * Blink
 * Turns on an LED on for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>

void setup()
{
  // initialize LED digital pin as an output.
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
  // turn the LED on (HIGH is the voltage level)
  digitalWrite(LED_BUILTIN, HIGH);
  // wait for a second
  delay(100);
  // turn the LED off by making the voltage LOW
  digitalWrite(LED_BUILTIN, LOW);
   // wait for a second
  delay(100);
}
