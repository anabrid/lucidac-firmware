#include <Arduino.h>
#include <cstring>

#include "distributor.h"
#include "carrier.h"
#include "client.h"
#include "logging.h"
#include "message_handlers.h"
#include "run.h"
#include "eeprom.h"
#include "serial_lines.h"
#include "user_auth.h"
#include "user_login.h"
#include "run_manager.h"

utils::SerialLineReader serial_line_reader;
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

  LOG_ALWAYS(dist::ident());

  LOG(ANABRID_DEBUG_INIT, "Loading settings from EEPROM...");
  settings.read_from_eeprom();

  LOG_START("UserPasswordAuthentification: "); LOG_JSON(settings.auth.status); LOG_END();

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
  msg::handlers::Registry::set("hack", new HackMessageHandler(), auth::SecurityLevel::RequiresNothing);
  msg::handlers::Registry::set("help", new msg::handlers::HelpHandler(), auth::SecurityLevel::RequiresNothing);
  msg::handlers::Registry::set("reset", new msg::handlers::ResetRequestHandler(carrier_), auth::SecurityLevel::RequiresLogin); // TODO: Should probably be called "reset_config" or so, cf. reset_settings
  msg::handlers::Registry::set("set_config", new msg::handlers::SetConfigMessageHandler(carrier_), auth::SecurityLevel::RequiresLogin);
  msg::handlers::Registry::set("get_config", new msg::handlers::GetConfigMessageHandler(carrier_), auth::SecurityLevel::RequiresLogin);
  msg::handlers::Registry::set("get_entities", new msg::handlers::GetEntitiesRequestHandler(carrier_), auth::SecurityLevel::RequiresLogin);
  msg::handlers::Registry::set("start_run", new msg::handlers::StartRunRequestHandler(run::RunManager::get()), auth::SecurityLevel::RequiresLogin);

  // TODO: It would be somewhat cleaner if the Hybrid Controller settings would be just part of the get_config/set_config idiom
  //   because with this notation, we double the need for setters and getters.
  msg::handlers::Registry::set("get_settings", new msg::handlers::GetSettingsHandler(settings), auth::SecurityLevel::RequiresAdmin);
  msg::handlers::Registry::set("update_settings", new msg::handlers::SetSettingsHandler(settings), auth::SecurityLevel::RequiresAdmin);
  msg::handlers::Registry::set("reset_settings", new msg::handlers::ResetSettingsHandler(settings), auth::SecurityLevel::RequiresAdmin);

  msg::handlers::Registry::set("status", new msg::handlers::GetSystemStatus(settings.auth), auth::SecurityLevel::RequiresNothing);
  msg::handlers::Registry::set("login", new msg::handlers::LoginHandler(settings.auth), auth::SecurityLevel::RequiresNothing);

  //LOG("msg::handlers::Registry set up with handlers")
  //msg::handlers::Registry::dump();

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}

bool handleMessage(JsonObjectConst envelope_in, JsonObject& envelope_out, auth::AuthentificationContext &user_context) {
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
  auto requiredClearance = msg::handlers::Registry::requiredClearance(msg_type);
  bool success = true;
  if (!msg_handler) {
    // No handler for message known
    success = false;
    msg_out["error"] = "Unknown message type. Try this message: {'type':'help'}.";
  } else if(!user_context.can_do(requiredClearance)) {
    success = false;
    msg_out["error"] = "User is not authorized for action";
  } else if(msg_type == "login") {
    success = ((msg::handlers::LoginHandler*)msg_handler)->
                           handle(envelope_in["msg"].as<JsonObjectConst>(), msg_out, user_context);
  } else {
    success = msg_handler->handle(envelope_in["msg"].as<JsonObjectConst>(), msg_out);
  }

  // Always include a success field for replies (out of band notifications won't run throught
  // this method, for example client::RunStateChangeNotificationHandler::handle)
  envelope_out["success"] = success;

  if (!success) {
    // Message could not be handled, mark envelope as unsuccessful
    envelope_out["error"] = msg_out["error"];
    envelope_out.remove("msg");
    LOG_ALWAYS("Error while handling message.");
  }

  // If message generated a response or an error, actually sent it out
  return (!msg_out.isNull() or !envelope_out["success"].as<bool>());
}


void process_serial_input() {
  char* line = serial_line_reader.line_available();
  if(!line) return;

  static auth::AuthentificationContext admin_context{settings.auth, auth::UserPasswordAuthentification::admin};
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
    auto envelope_out_obj = envelope_out.to<JsonObject>();
    if(handleMessage(envelope_in.as<JsonObjectConst>(), envelope_out_obj, admin_context)) {
      serializeJson(envelope_out_obj, Serial);
      Serial.println();
    }
  }
}

void loop() {
  net::EthernetClient connection = server.accept();
  auto &run_manager = run::RunManager::get();

  process_serial_input();

  if (connection) {

    Serial.print("Client connected from ");
    connection.remoteIP().printTo(Serial);
    Serial.println();

    auth::AuthentificationContext user_context{ settings.auth };
    user_context.set_remote_identifier( auth::RemoteIdentifier{ connection.remoteIP() } );

    // Reserve space for JSON communication
    // TODO: This must probably be global for all clients
    // TODO: Find out whether they should become rally global or really local (i.e. just before deserializeJson)
    DynamicJsonDocument envelope_in(4096), envelope_out(4096);

    // Bind things to this client specifically
    // TODO: This should also report to the Serial console when no connection takes place
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
        auto envelope_out_obj = envelope_out.to<JsonObject>();
        if(handleMessage(envelope_in.as<JsonObjectConst>(), envelope_out_obj, user_context)) {
          serializeJson(envelope_out_obj, Serial);
          serializeJson(envelope_out_obj, connection);
          if (!connection.writeFully("\n"))
            break;
        }
      }

      process_serial_input();

      // Fake run for now
      if (!run_manager.queue.empty()) {
        Serial.println("faking run");
        run_manager.run_next(&run_state_change_handler, &run_data_handler);
      }
    }

    // would be a good idea to move the fake run queue lines here.

    Serial.println("Client disconnected.");
  }
}
