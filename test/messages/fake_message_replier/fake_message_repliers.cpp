#include <Arduino.h>
#include <ArduinoJson.h>
#include <QNEthernet.h>

#include "message_handlers.h"

#define ERROR                                                                                                 \
  while (true) {                                                                                              \
    digitalToggle(13);                                                                                        \
    delay(100);                                                                                               \
  }

namespace net = qindesign::network;

uint16_t server_port = 5732;
net::EthernetServer server{server_port};

void setup() {
  Serial.begin(0);
  while (!Serial && millis() < 4000) {
    // Wait for Serial, but not forever
  }
  Serial.println("Hello.");

  if (!net::Ethernet.begin()) {
    ERROR
  }
  if (!net::Ethernet.waitForLocalIP(10000)) {
    ERROR
  } else {
    IPAddress ip = net::Ethernet.localIP();
    Serial.print("I am listening on ");
    ip.printTo(Serial);
    Serial.print(" port ");
    Serial.print(server_port);
    Serial.println();
  }

  server.begin();
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
        auto msg_handler = msg::handlers::MessageHandler::get(msg_type);
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
