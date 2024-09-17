// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#ifdef ARDUINO

#include "nvmconfig/persistent.h"

#include "net/auth.h"
#include "net/ethernet.h"
#include "nvmconfig/user.h"
#include "nvmconfig/vendor.h"

// Subsystems known so far (k/v):

// immutable: VendorOTP
// eth:       net::StartupConfig  [not permanent]
// server:    net::RuntimeConfig  <- no more, deleted
// auth:      net::auth::UserPasswordAuthentification
// user:      user-defined space irrelevant for the firmware
//            do not confuse this with the users dictionary in the auth!

namespace net {
/**
 * A registry for all permanent settings reachable for the protocol.
 *
 * Note how this registry does not own the settings since they are
 * self-owned Singletons.
 **/
inline void register_settings() {
  auto& persistent_settings = nvmconfig::PersistentSettingsWriter::get();
  auto &subsystems = persistent_settings.subsystems;
  subsystems.push_back(&nvmconfig::VendorOTP::get());
  subsystems.push_back(&nvmconfig::PermanentUserDefinedStuff::get());
  subsystems.push_back(&net::StartupConfig::get());
  subsystems.push_back(&net::auth::Gatekeeper::get());

  persistent_settings.read_from_eeprom();
}
} // namespace net

#endif // ARDUINO
