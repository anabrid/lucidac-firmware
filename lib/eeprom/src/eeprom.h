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
#include "user_auth.h"

namespace eeprom {

/**
 * Persistent user-changable configuration of the Teensy Microcontroller.
 * The persistency is realized via the EEPROM.
 * 
 * Values will be hold in the instance and synced to the EEPROM only via calls
 * to read_from_eeprom() or write_to_eeprom().
 **/
class UserSettings {
  uint64_t version;   ///< Version identifier on EEPROM to check validity

public:

  // Settings are managed in their relevant subclasses.
  // Here: network-related settings
  ethernet::UserDefinedEthernet ethernet;
  auth::UserPasswordAuthentification auth;
  // put other persistent entities here...

  void reset_defaults();   ///< reset to system default (also calls write_to_eeprom)

  void read_from_json(JsonObjectConst serialized_conf);
  void write_to_json(JsonObject target);

  void read_from_eeprom(); ///< read configuration from eeprom
  size_t write_to_eeprom();  ///< serialize configuration to eeprom, @returns consumed bytes in eeprom

  //UserSettings() { read_from_eeprom(); }

};

} // namespace eeprom


// TODO: I find this namespacing style really weird.
//       Why name a class "msg::handlers::FooRequestHandler" and not just "foo::FooRequestHandler"

namespace msg {

namespace handlers {

class SettingsHandler : public MessageHandler {
public:
  eeprom::UserSettings& settings;
  SettingsHandler(eeprom::UserSettings& settings) : settings(settings) {}
};

class GetSettingsHandler : public SettingsHandler {
public:
  using SettingsHandler::SettingsHandler;
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class SetSettingsHandler : public SettingsHandler {
public:
  using SettingsHandler::SettingsHandler;
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class ResetSettingsHandler : public SettingsHandler {
public:
  using SettingsHandler::SettingsHandler;
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers

} // namespace msg