// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "auth.h"

#ifdef ARDUINO

#include "utils/logging.h"
#include "net/auth.h"
#include "nvmconfig/vendor.h"

FLASHMEM void net::auth::Gatekeeper::reset_defaults() {
#ifdef ANABRID_UNSAFE_INTERNET
  enable_auth = false;
#else
  enable_auth = true;
#endif

  enable_users = true;
  enable_whitelist = true;

  access_control_allow_origin = "*";

  users.reset_defaults();
  whitelist.list.clear();
}

FLASHMEM void net::auth::UserPasswordAuthentification::reset_defaults() {
  db.clear();

  auto default_admin_password = nvmconfig::VendorOTP::get().default_admin_password;

  // "user" is just a non-admin default user which new ... users ... are supposed to
  // work with.
  auto default_user_password  = nvmconfig::VendorOTP::get().default_admin_password;

  if (!default_admin_password.empty())
    db[admin] = default_admin_password;
  if (!default_user_password.empty())
    db[user] = default_user_password;

  LOG_ALWAYS("UserPasswordAuthentification::reset_defaults() resetting admin and default user.");
}

FLASHMEM void net::auth::UserPasswordAuthentification::fromJson(JsonObjectConst serialized_conf) {
  db.clear();
  for (JsonPairConst kv : serialized_conf) {
    db[kv.key().c_str()] = kv.value().as<std::string>();
  }
}

FLASHMEM void net::auth::UserPasswordAuthentification::toJson(JsonObject target) const {
  for (auto const &kv : db) {
    target[kv.first] = kv.second;
  }
}

FLASHMEM void net::auth::UserPasswordAuthentification::status(JsonObject target) {
  target["enabled"] = !is_disabled();
  auto users = target.createNestedArray("users");
  for (auto const &kv : db)
    users.add(kv.first);
  // don't tell the passwords!
}

FLASHMEM int net::auth::Gatekeeper::login(JsonObjectConst msg_in, JsonObject &msg_out,
                                 net::auth::AuthentificationContext &user_context) {
  if (!enable_auth && !enable_users) {
    msg_out["error"] = "No authentification neccessary. Auth system is currently disabled (either by firmware build or user settings).";
    return 10;
  } else {
    std::string new_user = msg_in["user"];
    if (user_context.hasBetterClearenceThen(new_user)) {
      // This case avoids users from locking themself out of the serial console where they
      // always have admin permissions. It is not that interesting in a TCP/IP connection.
      msg_out["error"] =
          "Login can only upgrade privileges but you wold loose. Open a new connection instead.";
      return 1;
    } else if (!users.is_valid(new_user, msg_in["password"])) {
      msg_out["error"] = "Invalid username or password.";

      // todo: besseren DEBUG flag für finden, vgl.
      // https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/merge_requests/3#note_2361
      /*
      #if defined(ANABRID_DEBUG) || defined(ANABRID_DEBUG_INIT)
      // Show correct usernames and passwords on the console
      Serial.println("Since authentificiation failed and debug mode is on, this is the list of currently
      allowed passwords:");
      // TODO: Use LOG statements instead of Serial.print.
      StaticJsonDocument<500> kvmap;
      write_to_json(kvmap.to<JsonObject>());
      serializeJson(kvmap, Serial);
      Serial.println();
      #endif
      */

      return 2;
    } else {
      user_context.login(new_user);

      // TODO: Somehow this line doesn't show up.
      Serial.print("New authentification: ");
      user_context.printTo(Serial);
      Serial.println();

      return 0;
    }
  }
}

FLASHMEM int net::auth::Gatekeeper::lock_acquire(JsonObjectConst msg_in, JsonObject &msg_out,
                                        AuthentificationContext &user_context) {
  // Registry asserts that user is logged in, i.e. user_context.can_do(SecurityLevel::RequiresLogin)
  if (lock.is_locked()) {
    msg_out["error"] = "Computer is already locked"; // say by whom: lock.user
    return 1;
  } else {
    lock.enable_lock(user_context.user());
    return 0;
  }
}

FLASHMEM int net::auth::Gatekeeper::lock_release(JsonObjectConst msg_in, JsonObject &msg_out,
                                        AuthentificationContext &user_context) {
  if (!enable_users || user_context.user() == lock.holder ||
      user_context.can_do(SecurityLevel::RequiresAdmin)) {
    lock.force_unlock();
    return 0;
  }
  return 1;
}

#endif // ARDUINO
