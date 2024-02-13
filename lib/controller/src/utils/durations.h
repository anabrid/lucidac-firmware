#pragma once

#include <Arduino.h>

namespace utils {

  /// Simple time tracking (up to 50 days)
  struct duration {
    uint32_t last_time;
    void reset() { last_time = millis(); }
    duration() { reset(); }

    /// Returns true if period expired, false otherwise
    bool expired(unsigned long period) const {
        return (millis() - last_time >= period);
    }
  };

}
