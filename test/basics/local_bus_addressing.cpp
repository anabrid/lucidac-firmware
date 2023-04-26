#include <Arduino.h>
#include "local_bus.h"

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    local_bus::init();
}

void loop()
{
    local_bus::address_block(0, 5);
    digitalWriteFast(LED_BUILTIN, HIGH);
    for (uint32_t i = 0; i < 64; i ++) {
        local_bus::address_function(0, 5, i);
    }
    digitalWriteFast(LED_BUILTIN, LOW);
    local_bus::address_block(0, 0);
    delayMicroseconds(200);
}
