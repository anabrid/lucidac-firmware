// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "net/auth.h"
#include "protocol/handler.h"

// seperate header file from user_auth.h to avoid circular includes.

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class LoginHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext &user_context) override {
    return error(net::auth::Gatekeeper::get().login(msg_in, msg_out, user_context));
  }
};

/// @ingroup MessageHandlers
class LockAcquire : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext &user_context) override {
    return error(net::auth::Gatekeeper::get().lock_acquire(msg_in, msg_out, user_context));
  }
};

/// @ingroup MessageHandlers
class LockRelease : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext &user_context) override {
    return error(net::auth::Gatekeeper::get().lock_release(msg_in, msg_out, user_context));
  }
};

} // namespace handlers

} // namespace msg
