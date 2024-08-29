// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>

namespace utils {

using time_ms = uint32_t;

/// Simple time tracking (up to 50 days)
struct duration {
  time_ms last_time;

  void reset() { last_time = millis(); }

  duration() { reset(); }

  /// Returns true if period expired, false otherwise
  bool expired(time_ms period) const { return (millis() - last_time >= period); }
};

} // namespace utils
