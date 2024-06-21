// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

// The actual logging call
#ifdef ARDUINO
#include <Arduino.h>
#define __LOG(message)                                                                                        \
  Serial.println(message);                                                                                    \
  Serial.flush();
#else
#include <iostream>
#define __LOG(message) std::cerr << message;
#endif

// A logging macro, which accepts an optional LOG_FLAG (e.g. ANABRID_DEBUG_INIT) and a message.
#define LOG(LOG_FLAG, message) LOG_##LOG_FLAG(message)
#define LOG_ERROR(message) __LOG(message)
#define LOG_ALWAYS(message) __LOG(message)

// Unfortunately, we need to define the actual logging macro for each LOG_FLAG we want to use.
// Possibly, we can do some macro magic in the future.
// But probably not, because checking for the defined'ness of a macro inside another macro is not possible.

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
