// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <cstring>
#include <list>

#include "carrier/carrier.h"
#include "build/distributor.h"
#include "protocol/client.h"
#include "utils/logging.h"
#include "protocol/registry.h"
#include "protocol/handler.h"
#include "protocol/protocol.h"
#include "protocol/jsonl_server.h"
#include "net/auth.h"
#include "net/settings.h"
#include "net/settings.h"
#include "run/run_manager.h"
#include "utils/hashflash.h"
#include "web/server.h"

carrier::Carrier carrier_({Cluster(0)});

/// @ingroup MessageHandlers
class HackMessageHandler : public msg::handlers::MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    std::string command = msg_in["command"];
    if (command.empty())
      return 1;

    // Slave mode for transferring IC/OP signals
    if (command == "slave") {
      LOG_ALWAYS("Enabling slave mode, setting IC/OP pins to input (floating).");
      pinMode(mode::PIN_MODE_IC, INPUT);
      pinMode(mode::PIN_MODE_OP, INPUT);
    }

    return success;
  }
};

void setup() {
  // Initialize serial communication
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }

  LOG_ALWAYS(dist::ident());
  LOGV("Flash image (%d bytes) sha256 sum: %s\n",
      loader::flashimage::len(),
      loader::flashimage::sha256sum().to_string().c_str()
  );

  LOG(ANABRID_DEBUG_INIT, "Starting up Ethernet...");
  net::StartupConfig::get().begin();

  // Initialize carrier board
  // TODO, _ERROR_OUT_ shall not be used, see #116
  LOG(ANABRID_DEBUG_INIT, "Initializing carrier board...");
  if (!carrier_.init(net::StartupConfig::get().mac)) {
    LOG_ERROR("Error initializing carrier board.");
    _ERROR_OUT_
  }

  // Initialize things related to runs
  // TODO, _ERROR_OUT_ shall not be used, see #116
  if (!mode::FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME)) {
    LOG_ERROR("Error initializing FlexIO mode control.");
    _ERROR_OUT_
  }

  msg::handlers::Registry.init(carrier_); // registers all commonly known messages

  msg::handlers::Registry.set("hack", new HackMessageHandler(), net::auth::SecurityLevel::RequiresNothing);
  //LOG("msg::handlers::DynamicRegistry set up with handlers")
  //msg::handlers::DynamicRegistry::dump();

  msg::JsonLinesProtocol::get().init(4096); // Envelope size

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}

void loop() {
  msg::JsonlServer::get().loop();

  static net::auth::AuthentificationContext admin_context{net::auth::UserPasswordAuthentification::admin};
  msg::JsonLinesProtocol::get().process_serial_input(admin_context);

  web::LucidacWebServer::get().loop();

  // Currently, the following prints to all connected clients.
  static client::RunStateChangeNotificationHandler run_state_change_handler{
    msg::JsonLinesProtocol::get().broadcast,
    *msg::JsonLinesProtocol::get().envelope_out
  };
  static client::RunDataNotificationHandler run_data_handler{
    carrier_,
    msg::JsonLinesProtocol::get().broadcast,
    *msg::JsonLinesProtocol::get().envelope_out
  };

  if (!run::RunManager::get().queue.empty()) {
    Serial.println("faking run");
    run::RunManager::get().run_next(&run_state_change_handler, &run_data_handler);
  }
}
