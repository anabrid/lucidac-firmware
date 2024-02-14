// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <list>

#include "user/ethernet.h"
#include "utils/durations.h"

namespace web {
  struct LucidacWebServer {
    struct Client {
        utils::duration last_contact; ///< Tracking lifetime with millis() to time-out a connection.
        net::EthernetClient socket;
        //user::auth::AuthentificationContext user_context;
    };
    std::list<Client> clients;
    net::EthernetServer ethserver;

    static LucidacWebServer& get();
    void begin();
    void loop();
  };
} // namespace web