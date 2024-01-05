// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include <Ethernet.h>
#include <ArduinoJson.h>

#include "message_handlers.h"
#include "user/user_ethernet.h"
#include "user/user_auth.h"

namespace user {
namespace settings {

/**
 * Persistent user-changable configuration of the Teensy Microcontroller.
 * The persistency is realized via the EEPROM.
 * 
 * Values will be hold in the instance and synced to the EEPROM only via calls
 * to read_from_eeprom() or write_to_eeprom().
 **/
class JsonFlashUserSettings {
  uint64_t version;   ///< Version identifier on EEPROM to check validity

public:

  // Settings are managed in their relevant subclasses.
  user::ethernet::UserDefinedEthernet ethernet;
  user::auth::UserPasswordAuthentification auth;
  // put other persistent entities here...

  void reset_defaults();   ///< reset to system default (also calls write_to_eeprom)

  void read_from_json(JsonObjectConst serialized_conf);
  void write_to_json(JsonObject target);

  void read_from_eeprom(); ///< read configuration from eeprom
  size_t write_to_eeprom();  ///< serialize configuration to eeprom, @returns consumed bytes in eeprom

};

using UserSettingsImplementation = JsonFlashUserSettings;

} // namespace settings

extern settings::UserSettingsImplementation UserSettings;
} // namespace user


namespace msg {

namespace handlers {


class GetSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class SetSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

class ResetSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};

} // namespace handlers

} // namespace msg