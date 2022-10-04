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

#include <array>
#include <tuple>
#include "fmt/core.h"

#include <Arduino.h>
#include <EventResponder.h>
#include <SPI.h>

#undef max
#undef min
#include <cstdio>
#include <iostream>


extern "C" {
int _write(int fd, char *ptr, int len) {
  (void) fd;
  return Serial.write(ptr, len);
}
}

//
// CARE: Serial transmit buffer may overflow, making it look like things are stuck.
//

// Rough Speed limits, determined by measuring clock signal with oscilloscope 10x Probe
// ist aber auch etwas unklar, wie aussagekräftig das ist, weil das einfach in der Luft mit den Probes dran hängt.
// 20_000_000 ist ein ziemliches Dreieckssignal, max 3.1V, min 0.2V, aber ohne Überschwingen
// 10_000_000 kann man die Exponentialkurve noch gut sehen, aber er erreicht 3.3V und hat da auch ~30ns >= 3.0V
//            das sah irgendwie ganz realistisch aus, dass das eine nutzbare Geschwindigkeit ist
// 5_000_000 auch noch leicht mit exponentialkurve, ~80ns >= 3.0V
SPISettings spi_settings(10000000, MSBFIRST, SPI_MODE0);
EventResponder spi_dma_out_event;
volatile bool spi_in_progress = false;

std::array<uint8_t, 126000> data;



void spi_dma_out_event_callback(EventResponder& event_resp) {
    SPI.endTransaction();
    spi_in_progress = false;
    event_resp.clearEvent();
    std::cout << micros() << " SPI DONE" << std::endl;
}

void setup()
{
    Serial.begin(115200);

    // Initialize data array
    for (decltype(data)::size_type i = 0; i < data.size(); i++) {
        data[i] = i;
    }

    SPI.begin();
    spi_dma_out_event.attachImmediate(&spi_dma_out_event_callback);
}

void loop()
{
    delay(100);
    std::cout << micros() << " loop " << Serial.availableForWrite() << std::endl;

    if (!spi_in_progress && Serial.availableForWrite() > 0) {
        std::cout << micros() << " SPI START" << std::endl;
        spi_in_progress = true;
        SPI.beginTransaction(spi_settings);
        SPI.transfer(data.data(), nullptr, data.size(), spi_dma_out_event);
    }
}
