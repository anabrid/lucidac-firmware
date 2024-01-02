// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

// NOTE: Variadic macros and __PRETTY_FUNCTION__ are GNU specific extensions!
//       If you find __PRETTY_FUNCTION__ to verbose, see probably https://stackoverflow.com/a/64384924

// The actual logging call (but see also printf below)
#define __LOG(message) { Serial.print("# "); Serial.println(message); }

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


// moved here from main in order to use at other places
#define _ERROR_OUT_                                                                                           \
  while (true) {                                                                                              \
    digitalToggle(13);                                                                                        \
    delay(100);                                                                                               \
  }

// a bit more convenient logging

// Format Strings
#define LOGV(message,...) { Serial.printf("# " message "\n", __VA_ARGS__);  }
#define LOGMEV(message,...) { Serial.printf("#  %s: " message "\n", __PRETTY_FUNCTION__, __VA_ARGS__);  }

// stream oriented logging for "Printable" classes
#define LOG2(a, b) { Serial.print("# "); Serial.print(a); Serial.println(b); }
#define LOG3(a, b,c ) { Serial.print("# "); Serial.print(a); Serial.print(b); Serial.println(c); }
#define LOG4(a, b, c, d) { Serial.print("# "); Serial.print(a); Serial.print(b); Serial.print(c); Serial.println(d); }

// arbitrary length logging or "Printable" classes
#define LOG_START(message) { Serial.print("# "); Serial.print(message); }
#define LOG_ITEM(message) Serial.print(message);
#define LOG_END(message)  Serial.println(message);

// Log something which calls like foo(JsonObject& x) without hazzle, within LOG_START ... LOG_END.
#define LOG_JSON(functioncall) { StaticJsonDocument<200> tmp; functioncall(tmp.to<JsonObject>()); serializeJson(tmp, Serial); }
