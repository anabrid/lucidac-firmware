#pragma once

#include "user_auth.h"
#include "message_handlers.h"

// seperate header file from user_auth.h to avoid circular includes.

namespace msg {

namespace handlers {

class LoginHandler : public MessageHandler {
public:
  user::auth::UserPasswordAuthentification& auth;
  LoginHandler(user::auth::UserPasswordAuthentification& _auth) : auth(_auth) {}
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, user::auth::AuthentificationContext& user_context);
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) { return -1; } // never use this method.
};


} // namespace handlers

} // namespace msg