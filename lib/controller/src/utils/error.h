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
//extern char buf[bufsize];

template <typename... Args> FLASHMEM std::string small_sprintf(const char *format, Args... args) {
  char buf[bufsize];
  snprintf(buf, bufsize, format, args...);
  std::string ret = buf;
  return ret;
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

  status(int code, std::string msg) : code(code), msg(msg) {
    if(code!=0) {
      LOG4("utils::status error code=", code, " message=", msg.c_str());
      if(msg.length() == 0) {
        LOG_ALWAYS("Misuse of utils::status! Probably a casting problem.");
      }
    }
  }

  status() : status(0, "success") {} ///< Generic Success

  explicit status(int code) : status(code, "No error message provided") {}
  explicit status(bool code) : status(code ? 0 : 1) {}

  /// Generic failure (without code)
  explicit status(std::string msg) : status(-1, msg) {}

  // This constructor is neccessary to distinguish it from the status(*format, ...) constructor.
  explicit status(const char *msg) : status(-2, msg) {}

  /// Usage like status("Foo %d bar %s baz", 3, "bling");
  template <typename... Args>
  status(const char *format, Args... args) : status(small_sprintf(format, args...)) {
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
