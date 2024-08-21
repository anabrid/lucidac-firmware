#pragma once

#include "net/auth.h"
#include "protocol/handler.h"

// seperate header file from user_auth.h to avoid circular includes.

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class LoginHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext& user_context) {
    return error(net::auth::Gatekeeper::get().login(msg_in, msg_out, user_context));
  }
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) { return -1; } // never use this method.
};

/// @ingroup MessageHandlers
class LockAcquire : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext& user_context) {
    return error(net::auth::Gatekeeper::get().lock_acquire(msg_in, msg_out, user_context));
  }
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) { return -1; } // never use this method.
};

/// @ingroup MessageHandlers
class LockRelease : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext& user_context) {
    return error(net::auth::Gatekeeper::get().lock_release(msg_in, msg_out, user_context));
  }
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) { return -1; } // never use this method.
};


} // namespace handlers

} // namespace msg