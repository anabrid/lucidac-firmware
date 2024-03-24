// Copyright (c) 2024 anabrid GmbH
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

#pragma once

#include "protocol/handler.h"
#include "run/run_manager.h"

namespace msg {

namespace handlers {

class StartRunRequestHandler : public MessageHandler {
protected:
  run::RunManager &manager;

public:
  explicit StartRunRequestHandler(run::RunManager &run_manager) : manager(run_manager) {}

  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    if (!msg_in.containsKey("id") or !msg_in["id"].is<std::string>())
      return false;
    // Create run and put it into queue
    auto run = run::Run::from_json(msg_in);
    manager.queue.push(std::move(run));
    return true;
  };
};

} // namespace handlers

} // namespace msg
