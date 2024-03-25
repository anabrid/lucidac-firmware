#include "protocol/registry.h"

#include "utils/logging.h"
#include "utils/serial_lines.h"
#include "utils/durations.h"

#include "user/auth.h"
#include "user/settings.h"

#include "handlers/user_login.h"
#include "protocol.h"

#include <algorithm> 
#include <cctype>
#include <locale>


void trim(char *str) {
  int start = 0, end = strlen(str) - 1;

  // Remove leading whitespace
  while (isspace(str[start])) {
    start++;
  }

  // Remove trailing whitespace
  while (end > start && isspace(str[end])) {
    end--;
  }

  // If the string was trimmed, adjust the null terminator
  if (start > 0 || end < (strlen(str) - 1)) {
    memmove(str, str + start, end - start + 1);
    str[end - start + 1] = '\0';
  }
}

void msg::JsonLinesProtocol::init(size_t envelope_size) {
  envelope_in = new DynamicJsonDocument(envelope_size);
  envelope_out = new DynamicJsonDocument(envelope_size);
}

msg::JsonLinesProtocol &msg::JsonLinesProtocol::get() {
  static JsonLinesProtocol obj;
  return obj;
}

void msg::JsonLinesProtocol::handleMessage(user::auth::AuthentificationContext &user_context) {
  auto envelope_out = this->envelope_out->to<JsonObject>();
  auto envelope_in = this->envelope_in->as<JsonObjectConst>();

  // Unpack metadata from envelope
  std::string msg_id = envelope_in["id"];
  std::string msg_type = envelope_in["type"];

  // Create return envelope
  envelope_out.clear();
  envelope_out["id"] = msg_id;
  envelope_out["type"] = msg_type;
  auto msg_out = envelope_out.createNestedObject("msg");

  // Select message handler
  auto msg_handler = msg::handlers::Registry.get(msg_type);
  auto requiredClearance = msg::handlers::Registry.requiredClearance(msg_type);
  int return_code = 0;
  if (!msg_handler) {
    return_code = -10; // No handler for message known
    msg_out["error"] = "Unknown message type. Try this message: {'type':'help'}.";
  } else if(!user_context.can_do(requiredClearance)) {
    return_code = -20;
    msg_out["error"] = "User is not authorized for action";
  } else if(msg_type == "login") {
    return_code = ((msg::handlers::LoginHandler*)msg_handler)->
                           handle(envelope_in["msg"].as<JsonObjectConst>(), msg_out, user_context);
  } else {
    return_code = msg_handler->handle(envelope_in["msg"].as<JsonObjectConst>(), msg_out);
  }

  // Always include a success field for replies (out of band notifications won't run throught
  // this method, for example client::RunStateChangeNotificationHandler::handle)
  envelope_out["success"] = return_code == 0;

  if (return_code != 0) {
    // Message could not be handled, mark envelope as unsuccessful
    envelope_out["error"] = msg_out["error"];
    envelope_out["error_code"] = return_code;
    envelope_out.remove("msg");
    LOG_ALWAYS("Error while handling message.");
  }

  // If message generated a response or an error, actually sent it out
  // return (!msg_out.isNull() or !envelope_out["success"].as<bool>());
}

utils::SerialLineReader serial_line_reader;

void msg::JsonLinesProtocol::process_serial_input(user::auth::AuthentificationContext &user_context) {
  char* line = serial_line_reader.line_available();
  if(!line) return;

  auto error = deserializeJson(*envelope_in, line);
  if (error == DeserializationError::Code::EmptyInput) {
    // do nothing, just ignore empty input.
  } else if(error) {
    trim(line); // for not-destroying the output
    Serial.printf("# Serial input malformed: %s. Input was: '%s'\n", error.c_str(), line);
  } else {
    handleMessage(user_context);
    serializeJson(envelope_out->as<JsonObject>(), Serial);
    Serial.println();
  }
}

bool msg::JsonLinesProtocol::process_tcp_input(net::EthernetClient& connection, user::auth::AuthentificationContext &user_context) {
  auto error = deserializeJson(*envelope_in, connection);
  if (error == DeserializationError::Code::EmptyInput) {
    Serial.print(".");
  } else if (error) {
    Serial.print("Error while parsing JSON: ");
    Serial.println(error.c_str());
  } else {
    handleMessage(user_context);
    serializeJson(envelope_out->as<JsonObject>(), Serial);
    serializeJson(envelope_out->as<JsonObject>(), connection);
    if (!connection.writeFully("\n"))
      return true; // break;
  }
  return false;
}

void msg::JsonLinesProtocol::process_string_input(const std::string &envelope_in_str, std::string& envelope_out_str, user::auth::AuthentificationContext &user_context) {
  auto error = deserializeJson(*envelope_in, envelope_in_str);
  if (error == DeserializationError::Code::EmptyInput) {
    Serial.print(".");
  } else if (error) {
    envelope_out_str = "{'error':'Error while parsing JSON, error message: ";
    envelope_out_str += error.c_str();
    envelope_out_str += "'}\n";
  } else {
    handleMessage(user_context);
    serializeJson(envelope_out->as<JsonObject>(), Serial);
    serializeJson(envelope_out->as<JsonObject>(), envelope_out_str);
  }
}

void msg::MulticlientServer::loop() {
  net::EthernetClient client_socket = server->accept();

  if(client_socket) {
    // note that the following code copies "socket" object multiple times. When using pointers into
    // the struct, do not use the pointer to the wrong version.
    // TODO: Clean up code.
    Client client;
    client.socket = client_socket;
    client.user_context.set_remote_identifier( user::auth::RemoteIdentifier{ client_socket.remoteIP() } );

    // note there is also net::EthernetClient::maxSockets(), which is dominated by basic system constraints.
    // This limit here is rather dominated by application level constraints and can probably be a counter
    // measure against ddos attacks.
    if(clients.size() == user::UserSettings.ethernet.max_connections) {
      LOG5("Cannot accept client from ", client_socket.remoteIP(), " because maximum number of connections (",
        user::UserSettings.ethernet.max_connections, ") already reached.");
      client_socket.stop();
    } else {
      clients.push_back(client);
      JsonLinesProtocol::get().broadcast.add(&(std::prev(clients.end())->socket));
      LOG4("Client ", clients.size()-1, " connected from ", client_socket.remoteIP());
    }
  }

  // using iterator instead range-based loop because EthernetClient lacks == operator
  // so we use list::erase instead of list::remove.
  for(auto client = clients.begin(); client != clients.end(); client++) {
    const auto client_idx = std::distance(clients.begin(), client);
    if(client->socket) {
      if(client->socket.available() > 0) {
        msg::JsonLinesProtocol::get().process_tcp_input(client->socket, client->user_context);
        client->last_contact.reset();
      } else if(!client->socket.connected()) {
        client->socket.stop();
      } else if(client->last_contact.expired(user::UserSettings.ethernet.connection_timeout_ms)) {
        LOG5("Client ", client_idx, ", timed out after ", user::UserSettings.ethernet.connection_timeout_ms, " ms of idling");
        client->socket.stop();
      }
    } else {
      LOG5("Client ", client_idx, ", was ", client->user_context, ", disconnected");
      client->socket.close();
      JsonLinesProtocol::get().broadcast.remove(&client->socket);
      clients.erase(client);
      return; // iterator invalidated, better start loop() freshly.
    }
  }

}
