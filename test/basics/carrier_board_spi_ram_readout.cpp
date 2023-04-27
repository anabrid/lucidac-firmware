#include <Arduino.h>
#include "local_bus.h"

void setup()
{
    pinMode(LED_BUILTIN, OUTPUT);
    local_bus::init();
    local_bus::address_function(0, 5, 0);
}

void loop()
{
    digitalWriteFast(LED_BUILTIN, HIGH);
    // Set SPI configuration
    local_bus::spi.beginTransaction(SPISettings(4'000'000, MSBFIRST, SPI_MODE0));

    /*
     * Read out the full memory
     */
    // Set CS
    local_bus::address_function(0, 0, 0);
    // Send READ command
    local_bus::spi.transfer(B00000011);
    // Send Address
    local_bus::spi.transfer(B00000000);
    // Send 256*8 CLKs (data is irrelevant) to read DATA
    for (uint32_t i = 0; i < 256; i++) {
        local_bus::spi.transfer(B00000000);
    }
    // Unset CS
    local_bus::release_address();

    /*
     * Read out the 64bit unique ID memory at the end of the 256bit memory
     */
    delayMicroseconds(10);
    local_bus::address_function(0, 0, 0);
    // Send READ Command
    local_bus::spi.transfer(B00000011);
    // Send address of unique ID memory area
    local_bus::spi.transfer(248);
    // Send 8*8 CLKs (data is irrelevant) to read DATA
    for (uint32_t i = 0; i < 8; i++) {
        local_bus::spi.transfer(B00000000);
    }
    // Unset CS
    local_bus::release_address();

    local_bus::spi.endTransaction();
    digitalWriteFast(LED_BUILTIN, LOW);
    delay(200);
}
