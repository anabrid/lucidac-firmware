#include <Arduino.h>
#include <ArduinoJson.h>
#include <QNEthernet.h>

#include "carrier.h"
#include "client.h"
#include "logging.h"
#include "message_handlers.h"
#include "run.h"

#define _ERROR_OUT_                                                                                           \
  while (true) {                                                                                              \
    digitalToggle(13);                                                                                        \
    delay(100);                                                                                               \
  }

namespace net = qindesign::network;

uint16_t server_port = 5732;
net::EthernetServer server{server_port};

carrier::Carrier carrier_;

void setup() {
  // Initialize serial communication
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }
  LOG(ANABRID_DEBUG_INIT, "Hello.");

  // Initialize ethernet communication
  if (!net::Ethernet.begin(IPAddress(192, 168, 100, 222), IPAddress(255,255,255,0), IPAddress(192,168,100,1))) {
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
    LOG_ERROR("Error initializing carrier board.");
    _ERROR_OUT_
  }

  // Initialize things related to runs
  // ... Nothing yet :)
  if (!mode::FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME)) {
    LOG_ERROR("Error initializing FlexIO mode control.");
    _ERROR_OUT_
  }

  // Register message handler
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
        run_manager.run_next(&run_state_change_handler);
      }
    }

    Serial.println("Client disconnected.");
  }
}
