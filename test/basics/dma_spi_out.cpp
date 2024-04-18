// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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
