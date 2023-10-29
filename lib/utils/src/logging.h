// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#pragma once

// The actual logging call
#define __LOG(message) Serial.println(message);

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

// moved here from main in order to use at other places
#define _ERROR_OUT_                                                                                           \
  while (true) {                                                                                              \
    digitalToggle(13);                                                                                        \
    delay(100);                                                                                               \
  }
