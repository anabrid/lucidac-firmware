#include <Arduino.h>
#include <cstring>

#include "build/distributor.h"
#include "protocol/client.h"
#include "utils/logging.h"
#include "protocol/registry.h"
#include "protocol/handler.h"
#include "user/auth.h"
#include "user/settings.h"
#include "run/run_manager.h"

#include "utils/hashflash.h"

net::EthernetServer server;

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

  LOG(ANABRID_DEBUG_INIT, "Loading settings from flash...");
  user::UserSettings.read_from_eeprom();

  LOG_START("UserPasswordAuthentification: "); LOG_JSON(user::UserSettings.auth.status); LOG_END();

  LOG(ANABRID_DEBUG_INIT, "Starting up Ethernet...");
  user::UserSettings.ethernet.begin(&server);

  // Initialize carrier board
  LOG(ANABRID_DEBUG_INIT, "Initializing carrier board...");
  if (!carrier::Carrier::get().init(user::UserSettings.ethernet.mac)) {
    LOG_ERROR("Error initializing carrier board.");
    _ERROR_OUT_
  }

  // Initialize things related to runs
  // ... Nothing yet :)
  if (!mode::FlexIOControl::init(mode::DEFAULT_IC_TIME, mode::DEFAULT_OP_TIME)) {
    LOG_ERROR("Error initializing FlexIO mode control.");
    _ERROR_OUT_
  }

  msg::handlers::Registry.init(); // registers all commonly known messages

  msg::handlers::Registry.set("hack", new HackMessageHandler(), user::auth::SecurityLevel::RequiresNothing);
  //LOG("msg::handlers::DynamicRegistry set up with handlers")
  //msg::handlers::DynamicRegistry::dump();

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}


void loop() {
  net::EthernetClient connection = server.accept();
  auto &run_manager = run::RunManager::get();

  msg::process_serial_input();

  if (connection) {

    Serial.print("Client connected from ");
    connection.remoteIP().printTo(Serial);
    Serial.println();

    user::auth::AuthentificationContext user_context{ user::UserSettings.auth };
    user_context.set_remote_identifier( user::auth::RemoteIdentifier{ connection.remoteIP() } );

    // Reserve space for JSON communication
    // TODO: This must probably be global for all clients
    // TODO: Find out whether they should become rally global or really local (i.e. just before deserializeJson)
    DynamicJsonDocument envelope_in(4096), envelope_out(4096);

    // Bind things to this client specifically
    // TODO: This should also report to the Serial console when no connection takes place
    client::RunStateChangeNotificationHandler run_state_change_handler{connection, envelope_out};
    client::RunDataNotificationHandler run_data_handler{carrier::Carrier::get(), connection, envelope_out};

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
        int error_code = msg::handleMessage(envelope_in.as<JsonObjectConst>(), envelope_out_obj, user_context);
        //if(error_code == msg::handlers::MessageHandler::success) {
          serializeJson(envelope_out_obj, Serial);
          serializeJson(envelope_out_obj, connection);
          if (!connection.writeFully("\n"))
            break;
        //}
      }

      msg::process_serial_input();

      // Fake run for now
      if (!run::RunManager::get().queue.empty()) {
        Serial.println("faking run");
        run::RunManager::get().run_next(&run_state_change_handler, &run_data_handler);
      }
    }

    // would be a good idea to move the fake run queue lines here.

    Serial.println("Client disconnected.");
  }
}
