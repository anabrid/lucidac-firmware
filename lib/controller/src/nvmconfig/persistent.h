// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#ifdef ARDUINO

#include "utils/json.h"
#include "utils/singleton.h"
#include <list>

namespace nvmconfig {

/**
 * @brief JSON Conversion Context.
 *
 * This toggle allows to differentiate in fromJson/toJson which kind
 * of representation is wanted. It even allows to include or exclude
 * fields, as it is done in @file vendor.h where the information is
 * basically hidden to the user-side.
 **/
enum class Context {
  User, ///< User-Facing (writing/reading)
  Flash ///< Flash-Facing (writing/reading)
};

struct PersistentSettings {
  virtual std::string name() const = 0;
  virtual void reset_defaults() = 0;
  virtual void fromJson(JsonObjectConst src, Context c) = 0;
  virtual void toJson(JsonObject target, Context c) const = 0;

  /// Clear local memory in order to save RAM if an object is not
  /// needed during runtime but only at startup.
  virtual void clear() {}
};

// Teensy EEPROM is 4KB in size. This class claims the beginning
// of this storage. The required size is generously estimated, may be much less.
// Remember increasing the storage when more data have to be stored.
static constexpr int eeprom_address = 0, eeprom_size = 2'000;

// This number checks as a proof that the EEPROM stores something senseful. If it
// does not store a number >= this constant, it is resetted.
// Must not start from 0 in order to distinguish from "value not found" errors,
// thus the arbitrary nonzero 0xAA prefix.
static constexpr uint32_t required_magic = 0xAA03;

/**
 * Persistent user-changable configuration of the Teensy Microcontroller.
 * The persistency is realized via the EEPROM.
 *
 * Values will be hold in the instance and synced to the EEPROM only via calls
 * to read_from_eeprom() or write_to_eeprom().
 *
 * Note that, given that this class can use the more compact MessagePack
 * representation instead of classical JSON, the objects on the flash are
 * usually smaller then the ones transfered over network.
 *
 * \ingroup Singletons
 **/
class PersistentSettingsWriter : public utils::HeapSingleton<PersistentSettingsWriter> {
  uint32_t version; ///< Version identifier on EEPROM to check validity, must be bigger then required_magic
  static constexpr bool use_messagepack = false; ///< Use Messagepack instead of JSON on EEPROM

public:
  std::list<PersistentSettings *> subsystems;

  size_t write_to_eeprom(); ///< serialize configuration to eeprom, @returns consumed bytes in eeprom
  void read_from_eeprom();  ///< read configuration from eeprom

  // these methods just thread over subsystems
  void reset_defaults(bool write_to_eeprom = true);

  /**
   * This will only call the respective fromJson call in the subsystem
   * if its key is given in the object.
   */
  void fromJson(JsonObjectConst src, Context c);

  /**
   * Note that empty subsystems get removed, i.e.
   * if their toJson call doesn't fill the Object, it is omitted.
   */
  void toJson(JsonObject target, Context c);

  void info(JsonObject target);

  // Update configuration with a user call.
  // void set(JsonObjectConst serialized_conf, JsonObject& msg_out);
};

} // namespace nvmconfig

#endif // ARDUINO
