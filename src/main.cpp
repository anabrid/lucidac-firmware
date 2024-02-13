#include <Arduino.h>
#include <cstring>
#include <list>

#include "build/distributor.h"
#include "protocol/client.h"
#include "utils/logging.h"
#include "protocol/registry.h"
#include "protocol/handler.h"
#include "protocol/protocol.h"
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

  msg::JsonLinesProtocol::get().init(4096); // Envelope size

  // Done.
  LOG(ANABRID_DEBUG_INIT, "Initialization done.");
}

std::list<net::EthernetClient> clients;

void loop() {
  net::EthernetClient connection = server.accept();

  static user::auth::AuthentificationContext
    admin_context{user::UserSettings.auth, user::auth::UserPasswordAuthentification::admin},
    user_context {user::UserSettings.auth};

  uint32_t foo = millis();

  msg::JsonLinesProtocol::get().process_serial_input(admin_context);

  if (connection) {
    clients.push_back(connection);
    LOG4("Client ", clients.size()-1, " connected from ", connection.remoteIP());
  }

  // using iterator instead range-based loop because EthernetClient lacks == operator
  // so we use list::erase instead of list::remove.
  for(auto client = clients.begin(); client != clients.end(); client++) {
    const auto client_idx = std::distance(clients.begin(), client);
    if(*client) {
      if(client->available() > 0) {
        // have some data to process
        user_context.set_remote_identifier( user::auth::RemoteIdentifier{ client->remoteIP() } );
        msg::JsonLinesProtocol::get().process_tcp_input(*client, user_context);
      } else if(!client->connected()) {
        client->stop();
      }
    } else {
      LOG5("Client ", client_idx, ", was ", user_context, "disconnected");
      client->close();
      clients.erase(client);
      break; // iterator invalidated, better start fresh loop().
    }
  }

  /*
    
    // Bind things to this client specifically
    /// @todo This should also report to the Serial console when no connection takes place

    /// @todo This won't work any more with multiple connections. Runs need to remember the
    ///       session where they were started. Or otherwise report to all sessions.
    auto &envelope_out = *msg::JsonLinesProtocol::get().envelope_out;
    client::RunStateChangeNotificationHandler run_state_change_handler{connection, envelope_out};
    client::RunDataNotificationHandler run_data_handler{carrier::Carrier::get(), connection, envelope_out};


      msg::JsonLinesProtocol::get().process_serial_input(admin_context);

      // Fake run for now
      if (!run::RunManager::get().queue.empty()) {
        Serial.println("faking run");
        run::RunManager::get().run_next(&run_state_change_handler, &run_data_handler);
      }
    }

    // would be a good idea to move the fake run queue lines here.
  */
}
