// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <cstdarg>
#include <string>
#include "utils/logging.h"

namespace utils {
constexpr unsigned int bufsize = 1000;
extern char buf[bufsize];

template <typename... Args> FLASHMEM std::string small_sprintf(const char *format, Args... args) {
  snprintf(buf, bufsize, format, args...);
  return buf;
}

/**
 * A recoverable error, inspired from https://abseil.io/docs/cpp/guides/status
 * and https://github.com/abseil/abseil-cpp/blob/master/absl/status/status.h
 *
 * This class should be used as immutable.
 *
 * For making the success path cheap, use status::success.
 **/
class status {
public:
  int code;
  std::string msg;

  status(int code, std::string msg) : code(code), msg(msg) {}

  explicit status(int code) : code(code), msg("No error message provided") {}

  /// Generic failure (without code)
  status(std::string msg) : code(-1), msg(msg) {
    LOG2("utils::status generic failure ", msg.c_str());
  }

  status() : code(0) {} ///< Generic Success

  /// Usage like status("Foo %d bar %s baz", 3, "bling");
  template <typename... Args>
  status(const char *format, Args... args) : status(small_sprintf(format, args...)) {
    LOG2("utils::status: ", format);
  }

  template <typename... Args>
  status(int code, const char *format, Args... args) : status(code,  small_sprintf(format, args...)) {
    LOG3("utils::status with code ", code, format);
  }

  bool is_ok() const { return code == 0; }

  operator bool() const { return is_ok(); }

  /// Syntactic sugar
  static status success() { return status(0); }
};
} // namespace utils
