#pragma once

#include "ArduinoJson.h"
#include "user_auth.h"

namespace msg {
namespace protocol {

    /**
     * Handles a message, according to the registry.
     * Passes return codes as they appear from the handlers.
     * 
     * TODO: Could be a method of the Message Registry
     **/
    int handleMessage(JsonObjectConst envelope_in, JsonObject& envelope_out, user::auth::AuthentificationContext &user_context);

    void process_serial_input();


} // ns protocol
} // ns msg

