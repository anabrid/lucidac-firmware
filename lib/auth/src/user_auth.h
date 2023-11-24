// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <Printable.h>
#include <cstring>
#include <map>


namespace auth {

/**
 * Simple security levels for the message handlers
 **/
enum class SecurityLevel {
  RequiresAdmin,
  RequiresLogin,
  RequiresNothing
};

using User = std::string;

/**
 * A simple plaintext Username+Password authentification scheme backed against
 * the EEPROM UserSettings. It lacks group managament but treats the username
 * "admin" special as he can create new users.
 * No users in the db mean the authentification is turned off.
 * 
 * Class also to be used within the UserSettings, syncs both with EEPROM and
 * the distributor.
 **/
class UserPasswordAuthentification {
  std::map<User, std::string> db;
public:
  static constexpr const char* admin = "admin";

  bool is_disabled() const {
    #ifdef ANABRID_UNSAFE_INTERNET
      return true;
    #else
      return db.empty();
    #endif
  }
  bool is_valid(const User &user, const std::string &pwd) { return is_disabled() || (db.count(user) && db[user] == pwd); }

  bool can_do(const User& user, SecurityLevel task) const {
    if(is_disabled()) return true;
    if(task == SecurityLevel::RequiresAdmin) return user == admin;
    if(task == SecurityLevel::RequiresLogin) return !user.empty();
    /* if RequiresNothing */ return true;
  }

  void reset_defaults();   ///< reset to distributor defaults

  // note that these serializations deal with the sensitive user+password information
  // and are only used for eeprom::UserSettings.
  void read_from_json(JsonObjectConst serialized_conf);
  void write_to_json(JsonObject target);

  // if you want to let (any) remote users know something, use this
  void status(JsonObject target);
};

/// Some basic information about the remote station. We interpret 0.0.0.0 als a local terminal.
class RemoteIdentifier { // : public Printable { // no inheritance for POD
public:
  IPAddress ip;
  bool isLocal() const { return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0; } // (uint32_t)ip) doesnt work
	size_t printTo(Print& p) const {
    return isLocal() ? p.print("[serial-local]") : ip.printTo(p);
  }
};

/**
 * A thin wrapper around the username.
 * 
 * Note that we do not support logouts. Logout happens when the context object is destroyed.
 * The scope is typically given by the TCP/IP session.
 **/
class AuthentificationContext {
  UserPasswordAuthentification& auth;
  RemoteIdentifier _remote;
  User _user;
public:
  AuthentificationContext(UserPasswordAuthentification& auth, User user) : auth(auth), _user(user) {}
  AuthentificationContext(UserPasswordAuthentification& auth) : auth(auth) {}

  void set_remote_identifier(RemoteIdentifier r) { _remote = r; }

  bool can_do(SecurityLevel task) const { return auth.can_do(_user, task); }
  bool hasBetterClearenceThen(const User& other) const {
    return 
       (can_do(SecurityLevel::RequiresAdmin) && !auth.can_do(other, SecurityLevel::RequiresAdmin)) ||
       (can_do(SecurityLevel::RequiresLogin) && !auth.can_do(other, SecurityLevel::RequiresLogin));
  }
  void login(const User &user) { _user = user; }

	size_t printTo(Print& p) const {
    return p.print(_user.empty() ? "[nobody]" : _user.c_str()) + p.print("@") + _remote.printTo(p);
  }
};

} // namespace auth

