#include <Arduino.h>

#include "lucidac/front_panel_signaling.h"
#include "lucidac/lucidac.h"

FLASHMEM void leds::set(uint8_t val) {
  auto& carrier_ = platform::LUCIDAC::get();
  if(carrier_.front_panel) {
    carrier_.front_panel->leds.set_all(val);
    carrier_.front_panel->write_to_hardware();
  }
}

FLASHMEM void leds::indicate_error() {
  size_t num_blinks = 6;
  for(size_t i=0; i<num_blinks; i++) {
    leds::set(0x55);
    delay(100);
    leds::set(0xaa);
    delay(100);
  }
  leds::set(0);
}

FLASHMEM void leds::ease_out() {
  int val = 0xFF;
  for(uint8_t i=0; i<8; i++) {
    val /= 2; leds::set(val); delay(100);
  }
}