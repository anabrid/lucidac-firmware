// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#ifdef ARDUINO

#include <ArduinoJson.h>
#include <map>
#include <string>

#include "carrier/carrier.h"
#include "handler.h"
#include "net/auth.h"
#include "utils/singleton.h"

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
class DynamicRegistry : public utils::HeapSingleton<DynamicRegistry> {
  struct RegistryEntry { // "named tuple"
    MessageHandler *handler;
    net::auth::SecurityLevel clearance;
  };

  std::map<std::string, RegistryEntry> entries;

  int result_code_counter = 1, result_code_increment = 100;

public:
  MessageHandler *lookup(const std::string &msg_type); ///< Returns nullptr if not found
  net::auth::SecurityLevel requiredClearance(const std::string &msg_type);

  bool set(const std::string &msg_type, MessageHandler *handler, net::auth::SecurityLevel minimumClearance);
  bool set(const std::string &msg_type, int result_code_prefix, MessageHandler *handler,
           net::auth::SecurityLevel minimumClearance);

  void dump();                                    //< for debugging: Print Registry configuration to Serial
  void write_handler_names_to(JsonArray &target); ///< for structured output

  void init(carrier::Carrier &c); ///< Actual registration of all handlers in code.
};

using Registry = DynamicRegistry;

} // namespace handlers

} // namespace msg

#endif // ARDUINO
