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
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out, user::auth::AuthentificationContext& user_context);
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) { return false; } // never use this method.
};


} // namespace handlers

} // namespace msg