// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "utils/logging.h"
#include <Arduino.h>
#include <cstdarg>
#include <string>

namespace utils {
constexpr unsigned int bufsize = 1000;

// extern char buf[bufsize];

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
  int code;        ///< Status code. 0 means success.
  std::string msg; ///< Human readable error string.

  status(int code, std::string msg) : code(code), msg(msg) {
    if (code != 0) {
      LOG4("utils::status error code=", code, " message=", msg.c_str());
      if (msg.length() == 0) {
        LOG_ALWAYS("Misuse of utils::status! Probably a casting problem.");
      }
    }
  }

  /// Empty means success. See also success() method for more verbosity in code.
  status() : status(0, "success") {}

  /// 0 means success, anything else not success.
  explicit status(int code) : status(code, "No error message provided") {}

  /// True means success, false not success
  explicit status(bool code) : status(code ? 0 : 1) {}

  /// Generic failure (without code)
  explicit status(std::string msg) : status(-1, msg) {}

  // This constructor is neccessary to distinguish it from the status(*format, ...) constructor.
  explicit status(const char *msg) : status(-2, msg) {}

  /// Usage like status("Foo %d bar %s baz", 3, "bling");
  template <typename... Args>
  status(const char *format, Args... args) : status(small_sprintf(format, args...)) {}

  template <typename... Args>
  status(int code, const char *format, Args... args) : status(code, small_sprintf(format, args...)) {}

  /// Attach another error message to this one. Is chainable, returns self.
  status &attach(const status &other, const char *description = "") {
    if (!other.is_ok() || !is_ok()) {
      code += other.code * 128;
      msg += other.msg + description;
    }
    return *this; // chainable
  }

  /// Attach this error message to another one. Is chainable, returns self.
  status &attach_to(status &other, const char *description = "") {
    other.attach(*this, description);
    return *this;
  }

  /// Attach a "raw" message to this one.
  status &attach(int _code, const char *_msg) {
    code += _code * 128;
    msg += _msg;
    return *this;
  }

  status &attach(const char *_msg) {
    msg += _msg;
    return *this;
  }

  bool is_ok() const { return code == 0; }

  /// Class instances cast to bool where true means success and false means failure.
  operator bool() const { return is_ok(); }

  /// Syntactic sugar for success
  static status success() { return status(0); }

  /// Syntactic sugar for failure
  static status failure() { return status(-1); }
};
} // namespace utils
