// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <list>

#include "user/ethernet.h"
#include "utils/durations.h"

namespace web {

  /**
   * This class implements a simple webserver for the LUCIDAC.
   * It's main job is to elevate the JSONL protocol in the HTTP "REST-like" world,
   * allowing to speak the JSONL protocol also over HTTP and therefore opening
   * LUCIDAC programming to web technologies.
   * 
   * Second, it serves a few "static files", i.e. HTML/CSS/JS/images which allow to
   * host simple sites directly from the firmware (See also `assets.h` for the appropriate
   * tooling).
   * 
   * This web server shall have no further logic beyond passing queries to the standard
   * "protocol". Therefore, any "client side programs" are encoded in the "static files"
   * (also refered to as "assets"). For the firmware, this acts just as a regular client.
   * 
   **/
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