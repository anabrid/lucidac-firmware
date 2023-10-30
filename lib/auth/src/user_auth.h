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

#include <ArduinoJson.h>
#include <IPAddress.h>
#include <Printable.h>
#include <cstring>
#include <map>

#include "message_handlers.h"

namespace auth {

/**
 * Simple security levels for the message handlers
 **/
enum class SecurityLevel {
  RequiresAdmin,
  RequiresLogin,
  RequiresNothing
};

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
  std::map<std::string, std::string> db;
public:
  static constexpr const char* admin = "admin";

  bool isdisabled() const {  return db.empty(); }
  bool isvalid(std::string user, std::string pwd) { return isdisabled() || (db.count(user) && db[user] == pwd); }

  bool cando(std::string user, SecurityLevel task) const {
    if(isdisabled()) return true;
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

/// Some basic information about the remote station. Currently only distinguishes between
/// an IP and "no IP", which represents the local terminal
class RemoteIdentifier { // : public Printable {
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
  std::string _user;
public:
  AuthentificationContext(UserPasswordAuthentification& auth, std::string user) : auth(auth), _user(user) {}
  AuthentificationContext(UserPasswordAuthentification& auth) : auth(auth) {}

  void set_remote_identifier(RemoteIdentifier r) { _remote = r; }

  bool cando(SecurityLevel task) const { return auth.cando(_user, task); }
  bool hasBetterClearenceThen(std::string other) const {
    return 
       (cando(SecurityLevel::RequiresAdmin) && !auth.cando(other, SecurityLevel::RequiresAdmin)) ||
       (cando(SecurityLevel::RequiresLogin) && !auth.cando(other, SecurityLevel::RequiresLogin));
  }
  void login(std::string user) { _user = user; }

	size_t printTo(Print& p) const {
    return p.print(_user.empty() ? "[nobody]" : _user.c_str()) + p.print("@") + _remote.printTo(p);
  }
};

} // namespace auth


namespace msg {

namespace handlers {

class LoginHandler : public MessageHandler {
public:
  auth::UserPasswordAuthentification& auth;
  LoginHandler(auth::UserPasswordAuthentification& _auth) : auth(_auth) {}
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out, auth::AuthentificationContext& user_context);
  bool handle(JsonObjectConst msg_in, JsonObject &msg_out) { return false; } // never use this method.
};


} // namespace handlers

} // namespace msg