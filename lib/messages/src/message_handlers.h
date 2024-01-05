// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>

namespace msg {

namespace handlers {

class MessageHandler {
public:
  /**
   * @return Whether the handling was successful or not 
   **/
  virtual bool handle(JsonObjectConst msg_in, JsonObject &msg_out) = 0;
};


} // namespace handlers

} // namespace msg