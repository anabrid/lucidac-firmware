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

#pragma once

//#include "message_handlers.h"
#include "ArduinoJson.h"

/**
 * The distributor headerfile collects constants as C preprocessor headers.
 * They are composed at compile-time by a python script.
 * 
 * In the application context, some of these variables are considered secret (such as the
 * default admin password).
 * 
 * Since the preprocessor directives are not namespaced or typed (they are all C strings by
 * definition), some helpers functions are here as functions.
 * 
 **/
namespace dist {

  // Get a short oneline identifier name
  const char* ident();

  // Get the program memory database as already-serialized JSON String.
  const char* as_json(bool include_secrets=true);

  // Write the distributor database into an existing JSON document
  void write_to_json(JsonObject target, bool include_secrets=true);

} // namespace dist

#include "distributor_generated.h"

namespace msg {

namespace handlers {

// see messages/src/message_handlers.h for the msg::handlers::GetSystemStatus::handle
// which also includes the distributor information

/*
class GetDistributorHandler : public SettingsHandler {
public:
  using SettingsHandler::SettingsHandler;
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

*/

} // namespace handlers

} // namespace msg 
