// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

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
  ethernet::UserDefinedEthernet ethernet;
  auth::UserPasswordAuthentification auth;
  // put other persistent entities here...

  void reset_defaults();   ///< reset to system default (also calls write_to_eeprom)

  void read_from_json(JsonObjectConst serialized_conf);
  void write_to_json(JsonObject target);

  void read_from_eeprom(); ///< read configuration from eeprom
  size_t write_to_eeprom();  ///< serialize configuration to eeprom, @returns consumed bytes in eeprom

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