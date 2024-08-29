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
    /// Application level timeout, not to be confused with TCP-Level
    /// timout realized by QNEthernet. TCP Keepalive packages can keep
    /// a connection alive, but if no instructions arrive from clients,
    /// we want to close such connections to save ressources (the original
    /// motivation was to avoid blocking).
    ///
    /// Note that this locking mechanisms are bound to sessions which end
    /// when connections end, so this is also a method to avoid infinite
    /// locking ({type:lock} protocol messages).
    ///
    /// {type:ping} protocol messages can be used at application level to
    /// keep connections open.
    utils::duration last_contact;

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
