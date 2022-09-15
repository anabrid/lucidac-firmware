// Copyright (c) 2022 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#include <Arduino.h>


void setup()
{
    // This can also be replaced by register manipulation, but there is no real reason.
    for(auto pin: {23, 22, 21, 20, 19, 18, 17, 16})
        pinMode(pin, OUTPUT);

    // For more Pin information check Teensy documentation, primarily
    // https://www.pjrc.com/teensy/schematic.html
    // https://github.com/KurtE/TeensyDocuments/
}

void loop()
{
    delay(100);
    static bool value = true;
    value = !value;

    //for(auto pin: {23, 22, 21, 20, 19, 18, 17, 16})
    //    digitalWriteFast(pin, value);

    // See .platformio/packages/framework-arduinoteensy/cores/teensy4/core_pins.h
    // Or just check what `digitalWriteFast` does
    auto mask = CORE_PIN23_BITMASK | CORE_PIN22_BITMASK | CORE_PIN21_BITMASK | CORE_PIN20_BITMASK | CORE_PIN19_BITMASK | CORE_PIN18_BITMASK | CORE_PIN17_BITMASK | CORE_PIN16_BITMASK;
    if (value) {
        GPIO6_DR_SET = mask;
        // This does not need |= apparently
        //CORE_PIN23_PORTSET = CORE_PIN23_BITMASK;
    } else {
        GPIO6_DR_CLEAR = mask;
        //CORE_PIN23_PORTCLEAR = CORE_PIN23_BITMASK;
    }
}
