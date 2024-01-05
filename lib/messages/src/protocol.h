#pragma once

#include "ArduinoJson.h"
#include "user_auth.h"

namespace msg {
namespace protocol {

    bool handleMessage(JsonObjectConst envelope_in, JsonObject& envelope_out, user::auth::AuthentificationContext &user_context);

    void process_serial_input();


} // ns protocol
} // ns msg

