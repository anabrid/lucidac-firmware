// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

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