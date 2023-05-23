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

#include "message_handlers.h"

std::map<std::string, msg::handlers::MessageHandler *> msg::handlers::MessageHandler::handler_registry{
    {"ping", new PingRequestHandler{}},
    {"get_entities", new GetEntitiesRequestHandler{}}
};

msg::handlers::MessageHandler *msg::handlers::MessageHandler::get(std::string msg_type) {
  auto found = handler_registry.find(msg_type);
  if (found != handler_registry.end()) {
    return found->second;
  } else {
    return nullptr;
  }
}

bool msg::handlers::PingRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["now"] = 43;
  return false;
}

bool msg::handlers::GetEntitiesRequestHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out) {
  msg_out["06-00-00-00-00-00"] = serialized(R"({"blocks": {"0": {"class": 1, "type": 0, "variant": 0, "version": 0}, "1": {"class": 1, "type": 1, "variant": 0, "version": 0}, "2": {"class": 1, "type": 2, "variant": 0, "version": 0}, "3": {"class": 1, "type": 3, "variant": 0, "version": 0}, "4": {"class": 1, "type": 4, "variant": 0, "version": 0}}})");
  return true;
}
