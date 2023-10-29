#include <Arduino.h>

#include "carrier.h"
#include "client.h"
#include "logging.h"
#include "message_handlers.h"
#include "run.h"
#include "eeprom.h"

net::EthernetServer server;
carrier::Carrier carrier_;

eeprom::UserSettings settings;

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

  LOG(ANABRID_DEBUG_INIT, "LUCIDAC Hybrid Controller Firmware (dev branch)");

  LOG(ANABRID_DEBUG_INIT, "Loading settings from EEPROM...");
  settings.read_from_eeprom();

  LOG(ANABRID_DEBUG_INIT, "Starting up Ethernet...");
  settings.ethernet.begin(&server);

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
  msg::handlers::Registry::set("hack", new HackMessageHandler());
  msg::handlers::Registry::set("set_config", new msg::handlers::SetConfigMessageHandler(carrier_));
  msg::handlers::Registry::set("get_config", new msg::handlers::GetConfigMessageHandler(carrier_));
  msg::handlers::Registry::set("get_entities", new msg::handlers::GetEntitiesRequestHandler(carrier_));
  msg::handlers::Registry::set("start_run", new msg::handlers::StartRunRequestHandler(run::RunManager::get()));

  // TODO: It would be much cleaner if the Hybrid Controller configuration would be just part of the get_config/set_config idiom
  //   because with this notation, we double the setter/getter.
  //   Also, why can't we handle this with lambdas? That would avoid all that observer pattern boilerplate.
  msg::handlers::Registry::set("get_settings", new msg::handlers::GetSettingsHandler(settings));
  msg::handlers::Registry::set("update_settings", new msg::handlers::SetSettingsHandler(settings));
  msg::handlers::Registry::set("reset_settings", new msg::handlers::ResetSettingsHandler(settings));

  msg::handlers::Registry::set("eth_status", new msg::handlers::GetEthernetStatus(settings.ethernet, server));

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}

bool handleMessage(JsonObjectConst envelope_in, JsonObject& envelope_out) {
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

  Serial.println("within handleMessage, this is what msg_out is:");
  serializeJson(envelope_out, Serial);
  Serial.println("Now handing back");

  // If message generated a response or an error, actually sent it out
  return (!msg_out.isNull() or !envelope_out["success"].as<bool>());
}

// TODO: Clean up code, move to better places.
class SerialLineReader {
public:
  static constexpr int serial_buffer_size = 4096;
  char serial_buffer[serial_buffer_size];
  int serial_buffer_pos = 0;

  char* line_available() {
    if(Serial.available() > 0) {
      char in = Serial.read();

      // be a responsive console, give immediate feedback on typing.
      // TODO: User should be able to turn this off. Humans want this, machines not.
      Serial.print(in); Serial.flush();

      if(in == '\n') {
        serial_buffer[serial_buffer_pos] = '\0';
        serial_buffer_pos = 0;
        return serial_buffer;
      } else if (serial_buffer_pos < serial_buffer_size - 1) {  // Avoid buffer overflow
        serial_buffer[serial_buffer_pos++] = in;
      } else {
        // buffer is full, flush it anyway.
        serial_buffer[serial_buffer_pos-2] = '\n';
        serial_buffer[serial_buffer_pos-1] = '\0';
        serial_buffer_pos = 0;
        return serial_buffer;
      }
    }
    return NULL;
  }
};

SerialLineReader serial_line_reader; // TODO: Clean up code, move to better places.

void process_serial_input(char* line) {
  DynamicJsonDocument envelope_in(4096), envelope_out(4096);
  auto error = deserializeJson(envelope_in, line);
  if (error == DeserializationError::Code::EmptyInput) {
    // do nothing, just ignore empty input.
  } else if(error) {
    Serial.print("Serial input: Malformed JSON provided, input was: '");
    Serial.print(line); // strings with \r should be trimmed, they somewhat destroy the output.
    Serial.print("'. Error message: ");
    Serial.println(error.c_str());
  } else {
    auto envelope_out_obj = envelope_out.as<JsonObject>();
    if(handleMessage(envelope_in.as<JsonObjectConst>(), envelope_out_obj)) {
      serializeJson(envelope_out_obj, Serial);
      Serial.flush(); // or probably print a newline
    } else {
      Serial.print("Would not send this out:");
      serializeJson(envelope_out_obj, Serial);
    }
    envelope_out.clear();
  }
  envelope_in.clear();
}

void peek_serial_input() {
  char* line = serial_line_reader.line_available();
  if(line) process_serial_input(line);
}

void loop() {
  net::EthernetClient connection = server.accept();
  auto &run_manager = run::RunManager::get();

  peek_serial_input();

  if (connection) {
    Serial.print("Client connected from ");
    connection.remoteIP().printTo(Serial);
    Serial.println();


    // Reserve space for JSON communication
    // TODO: This must probably be global for all clients
    // TODO: Find out whether they should become rally global or really local (i.e. just before deserializeJson)
    DynamicJsonDocument envelope_in(4096), envelope_out(4096);

    // Bind things to this client specifically
    // TODO: This should also report to the Serial console when no connection takes place
    client::RunStateChangeNotificationHandler run_state_change_handler{connection, envelope_out};

    // Handle incoming messages
    while (connection) {
      envelope_in.clear();
      auto error = deserializeJson(envelope_in, connection);
      if (error == DeserializationError::Code::EmptyInput) {
        Serial.print(".");
      } else if (error) {
        Serial.print("Error while parsing JSON:");
        Serial.println(error.c_str());
      } else {
        auto envelope_out_obj = envelope_out.as<JsonObject>();
        if(handleMessage(envelope_in.as<JsonObjectConst>(), envelope_out_obj)) {
          serializeJson(envelope_out_obj, Serial);
          serializeJson(envelope_out_obj, connection);
          if (!connection.writeFully("\n"))
            break;
        }
      }

      peek_serial_input();

      // Fake run for now
      if (!run_manager.queue.empty()) {
        Serial.println("faking run");
        run_manager.run_next(&run_state_change_handler);
      }
    }

    // would be a good idea to move the fake run queue lines here.

    Serial.println("Client disconnected.");
  }
}
