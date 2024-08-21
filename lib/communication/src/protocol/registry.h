// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#ifdef ARDUINO

#include <ArduinoJson.h>
#include <map>
#include <string>

#include "carrier/carrier.h"
#include "handler.h"
#include "net/auth.h"

namespace msg {

namespace handlers {

/**
 * The Message Registry holds the list of all known message types.
 *
 * The DynamicRegistry implementation uses an heap-allocated std::map as well as
 * Handlers which can be null pointers.
 *
 * \ingroup Singletons
 **/
class DynamicRegistry {
  struct RegistryEntry { // "named tuple"
    MessageHandler *handler;
    net::auth::SecurityLevel clearance;
  };

  std::map<std::string, RegistryEntry> entries;

  int result_code_counter = 1, result_code_increment = 100;

public:
  MessageHandler *get(const std::string &msg_type);
  net::auth::SecurityLevel requiredClearance(const std::string &msg_type);

  bool set(const std::string &msg_type, MessageHandler *handler, net::auth::SecurityLevel minimumClearance);
  bool set(const std::string &msg_type, int result_code_prefix, MessageHandler *handler,
           net::auth::SecurityLevel minimumClearance);

  void dump();                                    //< for debugging: Print Registry configuration to Serial
  void write_handler_names_to(JsonArray &target); ///< for structured output

  void init(carrier::Carrier &c); ///< Actual registration of all handlers in code.
};

// the singleton instance
extern DynamicRegistry Registry;

} // namespace handlers

} // namespace msg

#endif // ARDUINO
