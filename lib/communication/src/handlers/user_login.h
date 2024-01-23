#pragma once

#include "user/auth.h"
#include "protocol/handler.h"

// seperate header file from user_auth.h to avoid circular includes.

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class LoginHandler : public MessageHandler {
public:
  user::auth::UserPasswordAuthentification& auth;
  LoginHandler(user::auth::UserPasswordAuthentification& _auth) : auth(_auth) {}
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, user::auth::AuthentificationContext& user_context) {
    return error(auth.login(msg_in, msg_out, user_context));
  }
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) { return -1; } // never use this method.
};


} // namespace handlers

} // namespace msg