// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <stdint.h>
#include <string>

namespace entities {

struct __attribute__((packed)) Version {
  const uint8_t major;
  const uint8_t minor;
  const uint8_t patch;

  constexpr Version(const uint8_t major_, const uint8_t minor_, const uint8_t patch_)
      : major(major_), minor(minor_), patch(patch_) {}

  constexpr Version(const uint8_t major_, const uint8_t minor_) : major(major_), minor(minor_), patch(0) {}

  constexpr explicit Version(const uint8_t major_) : major(major_), minor(0), patch(0) {}

  std::string to_string() const {
    return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
  }

  bool operator==(const Version &r) const {
    return this->major == r.major && this->minor == r.minor && this->patch == r.patch;
  }

  bool operator!=(const Version &r) const {
    return this->major != r.major || this->minor != r.minor || this->patch != r.patch;
  }

  operator bool() const { return major || minor || patch; }
};

inline bool operator<(const Version &l, const Version &r) {
  if (l.major != r.major)
    return l.major < r.major;
  if (l.minor != r.minor)
    return l.minor < r.minor;
  return l.patch < r.patch;
}

inline bool operator>(const Version &l, const Version &r) { return r < l; }

inline bool operator<=(const Version &l, const Version &r) { return !(r < l); }

inline bool operator>=(const Version &l, const Version &r) { return !(l < r); }

} // namespace entities
