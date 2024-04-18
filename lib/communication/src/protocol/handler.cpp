// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "handler.h"

std::map<std::string, msg::handlers::MessageHandler *> msg::handlers::Registry::_registry{
    {"ping", new PingRequestHandler{}}};

msg::handlers::MessageHandler *msg::handlers::Registry::get(const std::string &msg_type) {
  auto found = _registry.find(msg_type);
  if (found != _registry.end()) {
    return found->second;
  } else {
    return nullptr;
  }
}

bool msg::handlers::Registry::set(const std::string &msg_type, msg::handlers::MessageHandler *handler,
                                  bool overwrite) {
  auto found = _registry.find(msg_type);
  if (found != _registry.end() && !overwrite) {
    return false;
  } else {
    // TODO: Overwritten message handlers should be freed
    _registry[msg_type] = handler;
    return true;
  }
}

bool msg::handlers::PingRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["now"] = "2007-08-31T16:47+01:00";
  return false;
}
