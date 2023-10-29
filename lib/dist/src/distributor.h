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

#include <Ethernet.h>
#include <ArduinoJson.h>

#include "message_handlers.h"
#include "persistent_eth.h"
#include "uuid.h"
#include "version.h"

namespace dist {

/**
 * The distributor class collects ROM constants (built-in-flash constants) defined by the
 * firmware distributor. They are composed at compile-time by a python script.
 * 
 * In the application context, some of these variables are considered secret (such as the
 * default admin password).
 * 
 * The cumbersome function call-notation is used in order to require RAM only when
 * access is needed. This is only really relevant for the strings, but PROGMEM requires
 * a copy to RAM in order to be used as char* anyway, so yeah, this is where we are now.
 **/
class Distributor {
public:
    Distributor();

    String        oem_name() const;
    String        model_name() const;

    // device specific information
    String        appliance_serial_number() const;
    utils::UUID   appliance_serial_uuid() const;
    String        appliance_registration_link() const;

    utils::Version firmware_version() const;
    utils::Version protocol_version() const;

    String         default_admin_password() const;

    void write_to_json(JsonObject target) const;

};

} // namespace dist

namespace msg {

namespace handlers {

/*
class GetDistributorHandler : public SettingsHandler {
public:
  using SettingsHandler::SettingsHandler;
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

*/

} // namespace handlers

} // namespace msg 
