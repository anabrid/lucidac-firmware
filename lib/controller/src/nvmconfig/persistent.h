// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include "utils/json.h"
#include <list>

namespace nvmconfig {

struct PersistentSettings {
    virtual std::string name() const = 0;
    virtual void reset_defaults() = 0;
    virtual void fromJson(JsonObjectConst src) = 0;
    virtual void toJson(JsonObject target) const = 0;

    /// Clear local memory in order to save RAM if an object is not
    /// needed during runtime but only at startup.
    virtual void clear() {}
};

// Teensy EEPROM is 4KB in size. This class claims the beginning
// of this storage. The required size is generously estimated, may be much less.
// Remember increasing the storage when more data have to be stored.
static constexpr int
    eeprom_address = 0,
    eeprom_size = 2'000;

// This number checks as a proof that the EEPROM stores something senseful. If it
// does not store a number >= this constant, it is resetted.
// Must not start from 0 in order to distinguish from "value not found" errors,
// thus the arbitrary nonzero 0xAA prefix.
static constexpr uint64_t required_magic = 0xAA02;

/**
 * Persistent user-changable configuration of the Teensy Microcontroller.
 * The persistency is realized via the EEPROM.
 * 
 * Values will be hold in the instance and synced to the EEPROM only via calls
 * to read_from_eeprom() or write_to_eeprom().
 * 
 * \ingroup Singletons
 **/
class PersistentSettingsWriter {
  uint64_t version;                             ///< Version identifier on EEPROM to check validity
  static constexpr bool use_messagepack = true; ///< Use Messagepack instead of JSON on EEPROM

public:
  static PersistentSettingsWriter& get() {
    static PersistentSettingsWriter instance;
    return instance;
  }
  std::list<PersistentSettings*> subsystems;

  size_t write_to_eeprom();  ///< serialize configuration to eeprom, @returns consumed bytes in eeprom
  void read_from_eeprom();   ///< read configuration from eeprom

  // these methods just thread over subsystems
  void reset_defaults(bool write_to_eeprom=true);
  void fromJson(JsonObjectConst src);
  void toJson(JsonObject target);

  void info(JsonObject target);

  // Update configuration with a user call.
  // void set(JsonObjectConst serialized_conf, JsonObject& msg_out);

};


} // namespace nvmconfig


