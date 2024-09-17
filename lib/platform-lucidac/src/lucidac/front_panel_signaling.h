#pragma once

namespace leds {

/// Shorthand to set LEDs on LUCIDAC front panel
void set(uint8_t val);

/// Flashing all LEDs
/// Duration: 1.6sec blocking
void indicate_error();

/// Animation where all LEDs go off one by one
/// Duration: 800ms blocking
void ease_out();

} // namespace