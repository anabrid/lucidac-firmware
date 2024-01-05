// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>

namespace msg {

namespace handlers {

class MessageHandler {
public:
  constexpr static int success = 0;

  /**
   * Following the HRESULT idiom, this number is an assigned prefix by the registry.
   * It should always be addded to the return value of handle().
   * You can use the helper routine error(...) to simplify this.
   **/
  int result_prefix;
  constexpr int error(int internal_code) { return internal_code == success ? success : result_prefix + internal_code; }


  /**
   * @return An error code. 0 means the handling was successful, any nonzero
   *         value means something specific to the handler. Handlers should
   *         define their error values as constants.
   **/
  virtual int handle(JsonObjectConst msg_in, JsonObject &msg_out) = 0;
};


} // namespace handlers

} // namespace msg