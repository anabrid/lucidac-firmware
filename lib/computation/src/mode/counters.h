// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h> // uint32_t
#include "ArduinoJson.h"
#include "utils/singleton.h"

#include "mode/mode.h"

namespace mode {

/**
 * This class provides a simple way of doing some metrics about the device
 * usage since startup. Depending on the usage patterns of the lucidac, these
 * numbers can be meaningful (in particular with the very exact FlexIORunManager)
 * or not. In particular, the HALT total timer is most likely not very helpful.
 **/
class PerformanceCounter : public utils::HeapSingleton<PerformanceCounter> {
    elapsedMicros op, ic, halt; // Counters
    mode::Mode cur_mode; ///< Current/las mode

    uint32_t
        total_op_time_us,   ///< Total tracked OP time in microseconds
        total_ic_time_us,   ///< Total tracked IC time in microseconds
        total_halt_time_us, ///< Total tracked HALT time in microseconds
        total_number_of_runs;

public:
    PerformanceCounter() { reset(); }
    void reset();

    void to(mode::Mode new_mode); ///< Manual Mode tracking
    void add(mode::Mode some_mode, uint32_t usec);

    void increase_run();

    void to_json(JsonObject target);
};


}