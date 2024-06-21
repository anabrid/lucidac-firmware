// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Arduino.h>
#include <ArduinoJson.h>
#include <QNEthernet.h>

#include "carrier/carrier.h"
#include "logging.h"
#include "protocol/client.h"
#include "protocol/handlers.h"
#include "run/run.h"
#include "run/run_manager.h"

#define _ERROR_OUT_                                                                                           \
  pinMode(13, OUTPUT);                                                                                        \
  while (true) {                                                                                              \
    digitalToggle(13);                                                                                        \
    delay(100);                                                                                               \
  }

namespace net = qindesign::network;

uint16_t server_port = 5732;
net::EthernetServer server{server_port};

carrier::Carrier carrier_({Cluster(0)});

class HackMessageHandler : public msg::handlers::MessageHandler {
public:
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    std::string command = msg_in["command"];
    if (command.empty())
      return false;

    // Slave mode for transferring IC/OP signals
    if (command == "slave") {
      LOG_ALWAYS("Enabling slave mode, setting IC/OP pins to input (floating).");
      pinMode(mode::PIN_MODE_IC, INPUT);
      pinMode(mode::PIN_MODE_OP, INPUT);
    }

    return true;
  }
};

void setup() {
  // Initialize serial communication
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }

  bus::init();
  LOG(ANABRID_DEBUG_INIT, "Hello.");

  // Initialize ethernet communication
  if (!net::Ethernet.begin()) {
    LOG_ERROR("Error starting ethernet.");
    _ERROR_OUT_
  }
  LOG(ANABRID_DEBUG_INIT, "Waiting for IP address on ethernet...");
  if (!net::Ethernet.waitForLocalIP(10000)) {
    LOG_ERROR("Error getting IP address.");
    _ERROR_OUT_
  } else {
    __attribute__((unused)) IPAddress ip = net::Ethernet.localIP();
#ifdef ANABRID_DEBUG_INIT
    Serial.print("I am listening on ");
    ip.printTo(Serial);
    Serial.print(" port ");
    Serial.print(server_port);
    Serial.println();
#endif
  }
  server.begin();

  // Initialize carrier board
  LOG(ANABRID_DEBUG_INIT, "Initializing carrier board...");
  if (!carrier_.init()) {
    LOG_ERROR("Error: Unable to initialize carrier board.");
    _ERROR_OUT_
  }

  // Initialize things related to runs
  // ... Nothing yet :)
  if (!mode::FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME)) {
    LOG_ERROR("Error initializing FlexIO mode control.");
    _ERROR_OUT_
  }

  // Register message handler
  msg::handlers::Registry::set("hack", new HackMessageHandler());
  msg::handlers::Registry::set("reset", new msg::handlers::ResetRequestHandler(carrier_));
  msg::handlers::Registry::set("set_config", new msg::handlers::SetConfigMessageHandler(carrier_));
  msg::handlers::Registry::set("get_config", new msg::handlers::GetConfigMessageHandler(carrier_));
  msg::handlers::Registry::set("get_entities", new msg::handlers::GetEntitiesRequestHandler(carrier_));
  msg::handlers::Registry::set("start_run", new msg::handlers::StartRunRequestHandler(run::RunManager::get()));

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}

void loop() {
  net::EthernetClient connection = server.accept();
  auto &run_manager = run::RunManager::get();

  if (connection) {
    Serial.print("Client connected from ");
    connection.remoteIP().printTo(Serial);
    Serial.println();

    // Reserve space for JSON communication
    // TODO: This must probably be global for all clients
    DynamicJsonDocument envelope_in(4096);
    DynamicJsonDocument envelope_out(4096);

    // Bind things to this client specifically
    client::RunStateChangeNotificationHandler run_state_change_handler{connection, envelope_out};
    client::RunDataNotificationHandler run_data_handler{carrier_, connection, envelope_out};

    // Handle incoming messages
    while (connection) {
      auto error = deserializeJson(envelope_in, connection);
      if (error == DeserializationError::Code::EmptyInput) {
        Serial.print(".");
      } else if (error) {
        Serial.print("Error while parsing JSON:");
        Serial.println(error.c_str());
      } else {
        // Unpack metadata from envelope
        std::string msg_id = envelope_in["id"];
        std::string msg_type = envelope_in["type"];

        // Create return envelope
        envelope_out.clear();
        envelope_out["id"] = msg_id;
        envelope_out["type"] = msg_type;
        auto msg_out = envelope_out.createNestedObject("msg");

        // Select message handler
        auto msg_handler = msg::handlers::Registry::get(msg_type);
        if (!msg_handler) {
          // No handler for message known
          envelope_out["success"] = false;
          envelope_out["error"] = "Unknown message type.";
        } else {
          // Let handler handle message
          if (!msg_handler->handle(envelope_in["msg"].as<JsonObjectConst>(), msg_out)) {
            // Message could not be handled, mark envelope as unsuccessful
            envelope_out["success"] = false;
            envelope_out["error"] = msg_out["error"];
            envelope_out.remove("msg");
            Serial.println("Error while handling message.");
          }
        }

        // If message generated a response or an error, actually sent it out
        if (!msg_out.isNull() or !envelope_out["success"].as<bool>()) {
          serializeJson(envelope_out, Serial);
          serializeJson(envelope_out, connection);
          if (!connection.writeFully("\n"))
            break;
        }
      }

      // Fake run for now
      if (!run_manager.queue.empty()) {
        Serial.println("faking run");
        run_manager.run_next(&run_state_change_handler, &run_data_handler);
      }
    }

    Serial.println("Client disconnected.");
  }
}
