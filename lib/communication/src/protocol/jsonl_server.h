// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#ifdef ARDUINO

#include "net/auth.h"
#include "net/ethernet.h"
#include "utils/durations.h"
#include <list>

namespace msg {

/**
 * Formerly known as MulticlientServer, this implements a "multi-threading"
 * version of the simple TCP/IP "raw" server implementing the JSONL protocol.
 */
struct JsonlServer {
  struct Client {
    utils::duration last_contact; ///< Tracking lifetime with millis() to time-out a connection.
    net::EthernetClient socket;
    net::auth::AuthentificationContext user_context;
  };

  std::list<Client> clients;
  net::EthernetServer server;
  void loop();
  void begin();

  static JsonlServer &get() {
    static JsonlServer server;
    return server;
  }
};

} // namespace msg
#endif // ARDUINO
