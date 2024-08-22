// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

// TODO: Move "utils/logging.h" somewhere more appropriate, given the new dependency from the protocol

// NOTE: Variadic macros and __PRETTY_FUNCTION__ are GNU specific extensions!
//       If you find __PRETTY_FUNCTION__ to verbose, see probably https://stackoverflow.com/a/64384924

#include <Arduino.h>

#include <array>
#include <iostream>
#include <type_traits>
#include <vector>

#ifndef ARDUINO
#define LOG_TARGET Serial
#else
#include "protocol/jsonl_logging.h"
#define LOG_TARGET msg::Log::get()
#endif

// The actual logging call (but see also printf below)
// The actual logging call
#ifdef ARDUINO
#include "StreamUtils.h" // ArduinoSreamUtils
#define __LOG(message) LOG_TARGET.println(message);
#else
#define __LOG(message) std::cerr << message;
#endif

// A logging macro, which accepts an optional LOG_FLAG (e.g. ANABRID_DEBUG_INIT) and a message.
#define LOG(LOG_FLAG, message) LOG_##LOG_FLAG(message)
#define LOG_ERROR(message) __LOG(message)
#define LOG_ALWAYS(message) __LOG(message)

// Unfortunately, we need to define the actual logging macro for each LOG_FLAG we want to use.
// Possibly, we can do some macro magic in the future.
// But probably not, because checking for the defined'ness of a macro inside another macro is not possible.
// -> Reply: Just use if(LOG_FLAG), since LOG_FLAG is known at compile time, if(false) can be optimized away.

#ifdef ANABRID_DEBUG
#define LOG_ANABRID_DEBUG(message) __LOG(message)
#else
#define LOG_ANABRID_DEBUG(message) ((void)0)
#endif

#ifdef ANABRID_DEBUG_INIT
#define LOG_ANABRID_DEBUG_INIT(message) __LOG(message)
#else
#define LOG_ANABRID_DEBUG_INIT(message) ((void)0)
#endif

#ifdef ANABRID_DEBUG_STATE
#define LOG_ANABRID_DEBUG_STATE(message) __LOG(message)
#else
#define LOG_ANABRID_DEBUG_STATE(message) ((void)0)
#endif

#ifdef ANABRID_DEBUG_DAQ
#define LOG_ANABRID_DEBUG_DAQ(message) __LOG(message)
#else
#define LOG_ANABRID_DEBUG_DAQ(message) ((void)0)
#endif

#ifdef ANABRID_PEDANTIC
#define LOG_ANABRID_PEDANTIC(message) __LOG(message)
#else
#define LOG_ANABRID_PEDANTIC(message) ((void)0)
#endif

#ifdef ANABRID_DEBUG_CALIBRATION
#define LOG_ANABRID_DEBUG_CALIBRATION(message) __LOG(message)
#else
#define LOG_ANABRID_DEBUG_CALIBRATION(message) ((void)0)
#endif

// moved here from main in order to use at other places
#define _ERROR_OUT_                                                                                           \
  while (true) {                                                                                              \
    digitalToggle(13);                                                                                        \
    delay(100);                                                                                               \
  }

// a bit more convenient logging

// Format Strings
#define LOGV(message, ...)                                                                                    \
  { LOG_TARGET.printf("# " message "\n", __VA_ARGS__); }
#define LOGMEV(message, ...)                                                                                  \
  { LOG_TARGET.printf("#  %s: " message "\n", __PRETTY_FUNCTION__, __VA_ARGS__); }

// stream oriented logging for "Printable" classes
#define LOG2(a, b)                                                                                            \
  {                                                                                                           \
    LOG_TARGET.print("# ");                                                                                   \
    LOG_TARGET.print(a);                                                                                      \
    LOG_TARGET.println(b);                                                                                    \
  }
#define LOG3(a, b, c)                                                                                         \
  {                                                                                                           \
    LOG_TARGET.print("# ");                                                                                   \
    LOG_TARGET.print(a);                                                                                      \
    LOG_TARGET.print(b);                                                                                      \
    LOG_TARGET.println(c);                                                                                    \
  }
#define LOG4(a, b, c, d)                                                                                      \
  {                                                                                                           \
    LOG_TARGET.print("# ");                                                                                   \
    LOG_TARGET.print(a);                                                                                      \
    LOG_TARGET.print(b);                                                                                      \
    LOG_TARGET.print(c);                                                                                      \
    LOG_TARGET.println(d);                                                                                    \
  }
#define LOG5(a, b, c, d, e)                                                                                   \
  {                                                                                                           \
    LOG_TARGET.print("# ");                                                                                   \
    LOG_TARGET.print(a);                                                                                      \
    LOG_TARGET.print(b);                                                                                      \
    LOG_TARGET.print(c);                                                                                      \
    LOG_TARGET.print(d);                                                                                      \
    LOG_TARGET.println(e);                                                                                    \
  }

// arbitrary length logging or "Printable" classes
#define LOG_START(message)                                                                                    \
  {                                                                                                           \
    LOG_TARGET.print("# ");                                                                                   \
    LOG_TARGET.print(message);                                                                                \
  }
#define LOG_ITEM(message) LOG_TARGET.print(message);
#define LOG_END(message) LOG_TARGET.println(message);

// Log something which calls like foo(JsonObject& x) without hazzle, within LOG_START ... LOG_END.
// #define LOG_JSON(functioncall) { StaticJsonDocument<200> tmp; functioncall(tmp.to<JsonObject>());
// serializeJson(tmp, Serial); }

// #undef LOG_TARGET // was only an internal helper

template <typename T, size_t size> std::ostream &operator<<(std::ostream &os, const std::array<T, size> &arr) {
  if (arr.empty())
    os << "[ ]";
  else {
    os << "[ ";
    for (size_t i = 0; i < arr.size() - 1; i++) {
      if constexpr (std::is_same<T, uint8_t>::value)
        os << +arr[i] << ", ";
      else
        os << arr[i] << ",";
    }
    if constexpr (std::is_same<T, uint8_t>::value)
      os << +arr.back() << " ]";
    else
      os << arr.back() << " ]";
  }
  return os;
}

template <typename T> std::ostream &operator<<(std::ostream &os, const std::vector<T> &vec) {
  if (vec.empty())
    os << "[ ]";
  else {
    os << "[ ";
    for (size_t i = 0; i < vec.size() - 1; i++) {
      if constexpr (std::is_same<T, uint8_t>::value)
        os << +vec[i] << ", ";
      else
        os << vec[i] << ",";
    }
    if constexpr (std::is_same<T, uint8_t>::value)
      os << +vec.back() << " ]";
    else
      os << vec.back() << " ]";
  }

  return os;
}
