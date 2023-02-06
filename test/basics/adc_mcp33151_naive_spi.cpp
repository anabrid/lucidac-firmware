/*
 * Naive implementation of ADC SPI Protocol
 * Also turns on the LED.
 *    
 *    __
 * o-''|\_____/)
 *  \_/|_)     )
 *     \  __  /
 *     (_/ (_/   
 * doggo goes woof
 */

#include <Arduino.h>
#include "QNEthernet.h"


constexpr uint8_t N_CLOCKS = 32;

constexpr uint8_t PIN_LED = 13;
constexpr uint8_t PIN_CNVST = 6;
constexpr uint8_t PIN_CLK = 9;
constexpr uint8_t PIN_MISO = 10;

void setup() {
    pinMode(PIN_LED, OUTPUT);
    digitalWriteFast(PIN_LED, HIGH);

    pinMode(PIN_CNVST, OUTPUT);
    pinMode(PIN_CLK, OUTPUT);
    pinMode(PIN_MISO, INPUT);
}

void loop() {
    delayMicroseconds(25);
    digitalWriteFast(PIN_CNVST, HIGH);
    delayMicroseconds(1);
    digitalWriteFast(PIN_CNVST, LOW);
    delayMicroseconds(1);
    for (int i = 0; i < N_CLOCKS; i++) {
        digitalWriteFast(PIN_CLK, HIGH);
        delayNanoseconds(100);
        digitalReadFast(PIN_MISO);
        digitalWriteFast(PIN_CLK, LOW);
        delayNanoseconds(100);
    } 
}