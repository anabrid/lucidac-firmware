#include "protocol/registry.h"

#include "utils/logging.h"
#include "utils/serial_lines.h"

#include "user/auth.h"
#include "user/settings.h"

#include "handlers/user_login.h"


int msg::handleMessage(JsonObjectConst envelope_in, JsonObject& envelope_out, user::auth::AuthentificationContext &user_context) {
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
  return return_code;
}


utils::SerialLineReader serial_line_reader;

void msg::process_serial_input() {
  char* line = serial_line_reader.line_available();
  if(!line) return;

  static user::auth::AuthentificationContext admin_context{user::UserSettings.auth, user::auth::UserPasswordAuthentification::admin};
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
    int error_code = msg::handleMessage(envelope_in.as<JsonObjectConst>(), envelope_out_obj, admin_context);
    //if(error_code == msg::handlers::MessageHandler::success) {
      serializeJson(envelope_out_obj, Serial);
      Serial.println();
    //}
  }
}