// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

/**
 * @file Main entrance file for the LUCIDAC/REDAC firmware.
 * 
 * This file contains the Arduino-style setup() and loop()
 * functions and serves as main entrance point for the PlatformIO
 * `teensy41` environment (`pio run -e teensy41`). It includes
 * all the actual code from the `lib` directory.
 **/

#include <Arduino.h>
#include <cstring>
#include <list>

#include "lucidac/lucidac.h"
#include "build/distributor.h"
#include "protocol/client.h"
#include "utils/logging.h"
#include "protocol/registry.h"
#include "protocol/handler.h"
#include "protocol/jsonl_logging.h"
#include "protocol/protocol.h"
#include "protocol/jsonl_server.h"
#include "net/auth.h"
#include "net/settings.h"
#include "net/settings.h"
#include "run/run_manager.h"
#include "utils/hashflash.h"
#include "web/server.h"

platform::LUCIDAC carrier_;
auto& netconf = net::StartupConfig::get();

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

void setup_remote_log() {
  IPAddress remote{192,168,68,96};
  static EthernetClient client;
  client.connect(remote, 1234);
  msg::Log::get().sinks.add(&client);
}

void setup() {
  // Initialize serial communication
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }

  msg::Log::get().sinks.add(&Serial);
  msg::Log::get().sinks.add(&msg::StartupLog::get());

  bus::init();

  LOG_ALWAYS(dist::ident());
  LOGV("Flash image (%d bytes) sha256 sum: %s\n",
      loader::flashimage::len(),
      loader::flashimage::sha256sum().to_string().c_str()
  );

  LOG(ANABRID_DEBUG_INIT, "Starting up Ethernet...");
  netconf.begin();
  if(netconf.enable_mdns)
    netconf.begin_mdns();
  if(netconf.enable_jsonl)
    msg::JsonlServer::get().begin();
  if(netconf.enable_webserver)
    web::LucidacWebServer::get().begin();

  setup_remote_log();
  LOG_ALWAYS("Have set up remote log");

  // Initialize carrier board
  // TODO, _ERROR_OUT_ shall not be used, see #116
  LOG(ANABRID_DEBUG_INIT, "Initializing carrier board...");
  if (!carrier_.init()) {
    LOG_ERROR("Error initializing carrier board.");
    //_ERROR_OUT_
  }

/*
  for(;;) {
     Serial.println("Logging worked");
     msg::StartupLog::get().stream_to_json(Serial);
  }
*/

  // Initialize things related to runs
  // TODO, _ERROR_OUT_ shall not be used, see #116
  if (!mode::FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME)) {
    LOG_ERROR("Error initializing FlexIO mode control.");
    //_ERROR_OUT_
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
  if(netconf.enable_jsonl)
    msg::JsonlServer::get().loop();

  static net::auth::AuthentificationContext admin_context{net::auth::UserPasswordAuthentification::admin};
  msg::JsonLinesProtocol::get().process_serial_input(admin_context);

  if(netconf.enable_webserver)
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
