// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>

// Forward declaration to avoid dependency
namespace utils { struct StreamingJson; }
namespace net { namespace auth { struct AuthentificationContext; }}

namespace msg {

namespace handlers {

/**
 * A MessageHandler is a functor/closure like event handler.
 * Different method signatures are "probed" by the registry.
 **/
class MessageHandler {
public:
  constexpr static int not_implemented = -99;
  constexpr static int success = 0;

  /**
   * Following the HRESULT idiom, this number is an assigned prefix by the registry.
   * It should always be addded to the return value of handle().
   * You can use the helper routine error(...) to simplify this.
   **/
  int result_prefix = 0;
  constexpr int error(int internal_code) { return internal_code == success ? success : result_prefix + internal_code; }

  /**
   * The actual handler method gets a message and returns a message next to
   * the status code.
   * 
   * @return An error code. 0 means the handling was successful, any nonzero
   *         value means something specific to the handler. Handlers should
   *         define their error values as constants.
   **/
  virtual int handle(JsonObjectConst msg_in, JsonObject &msg_out) {
    return not_implemented;
  }

  /**
   * A handler variant that allows access to the authentification context of the client
   **/
  virtual int handle(JsonObjectConst msg_in, JsonObject &msg_out, net::auth::AuthentificationContext& user_context) {
    return not_implemented;
  }

  /**
   * Handler for streaming responses.
   **/
  virtual int handle(JsonObjectConst msg_in, utils::StreamingJson& msg_out) {
    return not_implemented;
  }

};


} // namespace handlers

} // namespace msg
