#include "protocol.h"

#ifdef ARDUINO

#include "protocol/registry.h"

#include "utils/StringPrint.h"
#include "utils/durations.h"
#include "utils/logging.h"
#include "utils/serial_lines.h"

#include "net/auth.h"
#include "net/settings.h"

#include "handlers/login_lock.h"
#include "protocol/protocol.h"
#include "protocol/protocol_oob.h"

#include "run/run_manager.h"

#include <algorithm>
#include <cctype>
#include <locale>

void trim(char *str) {
  unsigned int start = 0, end = strlen(str) - 1;

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

void msg::JsonLinesProtocol::handleMessage(net::auth::AuthentificationContext &user_context, Print &output) {
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
  } else if (!user_context.can_do(requiredClearance)) {
    return_code = -20;
    msg_out["error"] = "User is not authorized for action";
  } else {
    auto msg_in = envelope_in["msg"].as<JsonObjectConst>();
    return_code = msg_handler->handle(msg_in, msg_out);
    if (return_code == msg::handlers::MessageHandler::not_implemented) {
      return_code = msg_handler->handle(msg_in, msg_out, user_context);
      if (return_code == msg::handlers::MessageHandler::not_implemented) {
        // we don't use envelope_out but stream instead
        auto envelope_and_msg_out = utils::StreamingJson(output);
        envelope_and_msg_out.begin_dict();
        envelope_and_msg_out.kv("id", msg_id);
        envelope_and_msg_out.kv("type", msg_type);
        envelope_and_msg_out.key("msg");
        // it is now the job of the handler to wrap his content into
        // begin_dict() / end_dict() or begin_list() / end_list()
        return_code = msg_handler->handle(msg_in, envelope_and_msg_out);
        if (return_code != 0) {
          LOG_ALWAYS("Error while handling streaming message.")
        }
        envelope_and_msg_out.kv("code", return_code);
        envelope_and_msg_out.end_dict(); // envelope
        return;
      }
    }
  }

  if (return_code != 0) {
    // Message could not be handled, mark envelope as unsuccessful
    envelope_out["error"] = msg_out["error"];
    envelope_out.remove("msg");
    LOG_ALWAYS("Error while handling message.");
  }

  envelope_out["code"] = return_code;

  serializeJson(envelope_out, output);
  // notice we don't send a NL here, has to be done by the callee!
}

utils::SerialLineReader serial_line_reader;

void msg::JsonLinesProtocol::process_serial_input(net::auth::AuthentificationContext &user_context) {
  char *line = serial_line_reader.line_available();
  if (!line)
    return;

  auto error = deserializeJson(*envelope_in, line);
  if (error == DeserializationError::Code::EmptyInput) {
    // do nothing, just ignore empty input.
  } else if (error) {
    trim(line); // for not-destroying the output
    Serial.printf("### Malformed input. Expecting JSON-Lines. Error: %s. Input was: '%s'\n", error.c_str(),
                  line);
  } else {
    handleMessage(user_context, Serial);
    Serial.println();
  }
}

bool msg::JsonLinesProtocol::process_tcp_input(net::EthernetClient &connection,
                                               net::auth::AuthentificationContext &user_context) {
  auto error = deserializeJson(*envelope_in, connection);
  if (error == DeserializationError::Code::EmptyInput) {
    Serial.print(".");
  } else if (error) {
    Serial.print("Error while parsing JSON: ");
    Serial.println(error.c_str());
  } else {
    handleMessage(user_context, connection);
    // serializeJson(envelope_out->as<JsonObject>(), Serial);
    // serializeJson(envelope_out->as<JsonObject>(), connection);
    if (!connection.writeFully("\n"))
      return true; // break;
  }
  return false;
}

void msg::JsonLinesProtocol::process_string_input(const std::string &envelope_in_str,
                                                  std::string &envelope_out_str,
                                                  net::auth::AuthentificationContext &user_context) {
  auto error = deserializeJson(*envelope_in, envelope_in_str);
  if (error == DeserializationError::Code::EmptyInput) {
    Serial.print(".");
  } else if (error) {
    envelope_out_str = "{'error':'Error while parsing JSON, error message: ";
    envelope_out_str += error.c_str();
    envelope_out_str += "'}\n";
  } else {
    utils::StringPrint s;
    handleMessage(user_context, s);
    envelope_out_str = s.str();
    // serializeJson(envelope_out->as<JsonObject>(), Serial);
    // serializeJson(envelope_out->as<JsonObject>(), envelope_out_str);
  }
}

void msg::JsonLinesProtocol::process_out_of_band_handlers(carrier::Carrier &carrier_) {
  if (!run::RunManager::get().queue.empty()) {
    // Currently, the following prints to all connected clients.
    client::RunStateChangeNotificationHandler run_state_change_handler{broadcast, *envelope_out};
    client::RunDataNotificationHandler run_data_handler{carrier_, broadcast};
    client::StreamingRunDataNotificationHandler alternative_run_data_handler{carrier_, broadcast};

    // TODO: Remove after debugging
    // LOGMEV("Protocol OOB RunManager now broadcasting to %d targets\n", broadcast.size());
    // broadcast.println("{'TEST':'TEST'}");
    run::RunManager::get().run_next(&run_state_change_handler, &run_data_handler, &alternative_run_data_handler);
  }
}

#endif // ARDUINO
