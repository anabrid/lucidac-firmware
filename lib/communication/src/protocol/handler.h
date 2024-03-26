// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>

// Forward declaration to avoid dependency
namespace user { namespace auth { struct AuthentificationContext; }}

namespace msg {

namespace handlers {

/**
 * A MessageHandler is a kind of event handler interface that acts like a closure.
 * That is, the class itself is abstract.
 **/
class MessageHandler {
public:
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
  virtual int handle(JsonObjectConst msg_in, JsonObject &msg_out) = 0;
};


} // namespace handlers

} // namespace msg
