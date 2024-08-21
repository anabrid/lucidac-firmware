// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#ifdef ARDUINO

#include <IPAddress.h>
#include <Printable.h>
#include <cstring>
#include <list>
#include <map>

#include "utils/durations.h"
#include "utils/json.h"
#include "utils/mac.h"

#include "nvmconfig/persistent.h"

namespace net {

/**
 * The net::auth namespace collects any security-related measures to
 * harden the LUCIDAC device in the wild and open Ethernet/IPv4 land.
 * These measures are not supposed to be integrated into REDAC because
 * it is supposed to be a trusted/controlled network.
 **/
namespace auth {

/**
 * Simple security levels for the message handlers
 **/
enum class SecurityLevel { RequiresAdmin, RequiresLogin, RequiresNothing };

using User = std::string;

class AuthentificationContext; // defined below

/**
 * A simple plaintext Username+Password authentification scheme backed against
 * the EEPROM UserSettings. It lacks group managament but treats the username
 * "admin" special as he can create new users.
 * No users in the db mean the authentification is turned off.
 *
 * Class also to be used within the UserSettings, syncs both with EEPROM and
 * the distributor.
 *
 * \ingroup Singletons
 **/
class UserPasswordAuthentification {
  // bool enable_authentification;
  std::map<User, std::string> db;

public:
  static constexpr const char *admin = "admin";

  void reset_defaults(); ///< reset to distributor defaults

  bool is_disabled() const { return /* !enable_authentification ||*/ db.empty(); }

  bool is_valid(const User &user, const std::string &pwd) {
    return /*is_disabled() ||*/ (db.count(user) && db[user] == pwd);
  }

  bool can_do(const User &user, SecurityLevel task) const {
    // if(is_disabled()) return true;
    if (task == SecurityLevel::RequiresAdmin)
      return user == admin;
    if (task == SecurityLevel::RequiresLogin)
      return !user.empty();
    /* if RequiresNothing */ return true;
  }

  // note that these serializations deal with the sensitive user+password information
  // and are only used for eeprom::UserSettings.
  void fromJson(JsonObjectConst serialized_conf);
  void toJson(JsonObject target) const;

  // if you want to let (any) remote users know something, use this
  void status(JsonObject target);
};

JSON_CONVERT_SUGAR(UserPasswordAuthentification);

/// Some basic information about the remote station. We interpret 0.0.0.0 als a local terminal.
class RemoteIdentifier { // : public Printable { // no inheritance for POD
public:
  IPAddress ip;

  bool isLocal() const {
    return ip[0] == 0 && ip[1] == 0 && ip[2] == 0 && ip[3] == 0;
  } // (uint32_t)ip) doesnt work

  size_t printTo(Print &p) const { return isLocal() ? p.print("[serial-local]") : ip.printTo(p); }
};

/**
 * Facilities to lock a LUCIDAC device by a single user.
 **/
struct DeviceLock {
  User holder;
  utils::duration locked_at;
  static constexpr utils::time_ms max_lock_duration = 3 * 60 * 1000; // 3min...

  void check() {
    if (locked_at.expired(max_lock_duration))
      /* force_unlock(): */ holder.clear();
  }

  bool is_locked() {
    check();
    return !holder.empty();
  }

  void force_unlock() { holder.clear(); }

  void enable_lock(const User &user) {
    holder = user;
    locked_at.reset();
  }
};

/**
 * Simple failed login backoff to avoid login brute force attempts.
 *
 * Note: The using code shall check on RemoteIdentifier::isLocal() and not attemp
 *       this stragey there.
 **/
class FailToBan {
  uint8_t max_known_stations = 20;
  constexpr static utils::time_ms max_waiting_time = 10 * 1000; // 10sec

  struct EndpointInformation {
    IPAddress ip;
    utils::duration last_failure;
    uint8_t failures = 0;
  };

  std::list<EndpointInformation> endpoints;

  void clean() {
    if (endpoints.size() >= max_known_stations)
      endpoints.pop_front();
    endpoints.remove_if([](EndpointInformation &a) { return a.last_failure.expired(max_waiting_time); });
  }

  EndpointInformation *find(const IPAddress &ip) {
    for (auto &e : endpoints)
      if (e.ip == ip)
        return &e;
    return nullptr;
  }

public:
  utils::time_ms failure_time(const IPAddress &ip) {
    clean();
    EndpointInformation *e;
    uint8_t failures = 1;
    if ((e = find(ip)))
      failures = ++e->failures;
    else
      endpoints.push_back({ip, utils::duration(), failures});
    return failures * failures * 100; // roughly 1sec after 3 attempts then up to 10secs timeout
  }
};

/**
 * Actually carries out login.
 * Keeps track of device locking and failed logins.
 * Owner of the UserPasswordAuthentification
 *
 * \ingroup Singleton
 */
struct Gatekeeper : nvmconfig::PersistentSettings {
  bool enable_auth,     ///< Overall switch to enable/disable security hardness
      enable_users,     ///< Enable/disable login at all, i.e. the user-password authentification
      enable_whitelist; ///< Enable/disable the IP Whitelist lookup

  UserPasswordAuthentification users;
  utils::IPMaskList whitelist;

  DeviceLock lock;
  FailToBan ban;

  std::string access_control_allow_origin; ///< For webserver: CORS setting
  // TODO, check also websocket_upgrade Origin field.

  /// Carrys out an actual login
  int login(JsonObjectConst msg_in, JsonObject &msg_out, AuthentificationContext &user_context);
  int lock_acquire(JsonObjectConst msg_in, JsonObject &msg_out, AuthentificationContext &user_context);
  int lock_release(JsonObjectConst msg_in, JsonObject &msg_out, AuthentificationContext &user_context);

  static Gatekeeper &get() {
    static Gatekeeper instance;
    return instance;
  }

  std::string name() const { return "auth"; }

  void reset_defaults();

  void fromJson(JsonObjectConst src, nvmconfig::Context c = nvmconfig::Context::Flash) override {
    JSON_GET(src, enable_auth);
    JSON_GET(src, enable_users);
    JSON_GET(src, enable_whitelist);
    JSON_GET(src, users);
    JSON_GET(src, whitelist);
    JSON_GET_AS(src, access_control_allow_origin, std::string);
  }

  void toJson(JsonObject target, nvmconfig::Context c = nvmconfig::Context::Flash) const override {
    JSON_SET(target, enable_auth);
    JSON_SET(target, enable_users);
    JSON_SET(target, enable_whitelist);
    JSON_SET(target, users);
    JSON_SET(target, whitelist);
    JSON_SET(target, access_control_allow_origin);
  }
};

JSON_CONVERT_SUGAR(Gatekeeper);

/**
 * A thin wrapper around the username.
 *
 * Note that we do not support logouts. Logout happens when the context object is destroyed.
 * The scope is typically given by the TCP/IP session.
 **/
class AuthentificationContext : public Printable {
  static UserPasswordAuthentification &auth() { return Gatekeeper::get().users; }

  RemoteIdentifier _remote;
  User _user;

public:
  AuthentificationContext(User user) : _user(user) {}

  AuthentificationContext() {}

  void set_remote_identifier(RemoteIdentifier r) { _remote = r; }

  bool can_do(SecurityLevel task) const {
    // TODO Also implement IP-based access here
    return !Gatekeeper::get().enable_auth || !Gatekeeper::get().enable_users || auth().can_do(_user, task);
  }

  bool hasBetterClearenceThen(const User &other) const {
    return (can_do(SecurityLevel::RequiresAdmin) && !auth().can_do(other, SecurityLevel::RequiresAdmin)) ||
           (can_do(SecurityLevel::RequiresLogin) && !auth().can_do(other, SecurityLevel::RequiresLogin));
  }

  void login(const User &user) { _user = user; }

  User user() const { return _user.empty() ? "[nobody]" : _user; }

  size_t printTo(Print &p) const override {
    return p.print(user().c_str()) + p.print("@") + _remote.printTo(p);
  }
};

} // namespace auth
} // namespace net

#endif // ARDUINO
