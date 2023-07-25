#include <Arduino.h>
#include <ArduinoJson.h>
#include <QNEthernet.h>

#include "carrier.h"
#include "logging.h"
#include "message_handlers.h"

#define ERROR                                                                                                 \
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
  if (!net::Ethernet.begin()) {
    LOG_ERROR("Error starting ethernet.");
    ERROR
  }
  LOG(ANABRID_DEBUG_INIT, "Waiting for IP address on ethernet...");
  if (!net::Ethernet.waitForLocalIP(10000)) {
    LOG_ERROR("Error getting IP address.");
    ERROR
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
    ERROR
  }
  // Register message handler
  msg::handlers::Registry::set("set_config", new msg::handlers::SetConfigMessageHandler(carrier_));
  msg::handlers::Registry::set("get_config", new msg::handlers::GetConfigMessageHandler(carrier_));

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}

void loop() {
  net::EthernetClient client = server.accept();

  if (client) {
    Serial.print("Client connected from ");
    client.remoteIP().printTo(Serial);
    Serial.println();

    DynamicJsonDocument envelope_in(2048);
    DynamicJsonDocument envelope_out(2048);
    while (client) {
      auto error = deserializeJson(envelope_in, client);
      if (error == DeserializationError::Code::EmptyInput) {
        Serial.print(".");
        continue;
      } else if (error) {
        Serial.print("Error while parsing JSON:");
        Serial.println(error.c_str());
        continue;
      } else {
        // Unpack metadata from envelope
        unsigned int msg_id = envelope_in["_id"];
        std::string msg_type = envelope_in["_type"];

        // Create return envelope
        envelope_out.clear();
        envelope_out["_id"] = msg_id;
        envelope_out["_type"] = msg_type;
        auto msg_out = envelope_out.createNestedObject("msg");

        // Select message handler
        auto msg_handler = msg::handlers::Registry::get(msg_type);
        if (!msg_handler) {
          // No handler for message known
          envelope_out["_success"] = false;
          msg_out["error"] = "Unknown message type.";
        } else {
          // Let handler handle message
          if (!msg_handler->handle(envelope_in["msg"].as<JsonObjectConst>(), msg_out)) {
            // Message could not be handled, mark envelope as unsuccessful
            envelope_out["_success"] = false;
            Serial.println("Error while handling message.");
          }
        }

        // If message generated a response or an error, actually sent it out
        if (!msg_out.isNull() or envelope_out["_success"].as<bool>()) {
          serializeJson(envelope_out, Serial);
          serializeJson(envelope_out, client);
          if (!client.writeFully("\n"))
            break;
        }
      }
    }

    Serial.println("Client disconnected.");
  }
}
