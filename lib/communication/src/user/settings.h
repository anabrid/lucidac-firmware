// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Ethernet.h>
#include <ArduinoJson.h>

#include "user/ethernet.h"
#include "user/auth.h"

namespace user {
namespace settings {

/**
 * Persistent user-changable configuration of the Teensy Microcontroller.
 * The persistency is realized via the EEPROM.
 * 
 * Values will be hold in the instance and synced to the EEPROM only via calls
 * to read_from_eeprom() or write_to_eeprom().
 * 
 * \ingroup Singletons
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

  // Update configuration with a user call.
  void set(JsonObjectConst serialized_conf, JsonObject& msg_out);

  void read_from_eeprom(); ///< read configuration from eeprom
  size_t write_to_eeprom();  ///< serialize configuration to eeprom, @returns consumed bytes in eeprom

};

using UserSettingsImplementation = JsonFlashUserSettings;

} // namespace settings

extern settings::UserSettingsImplementation UserSettings;
} // namespace user

