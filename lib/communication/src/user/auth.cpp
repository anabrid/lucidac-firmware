#include "user/auth.h"
#include "build/distributor.h"
#include "auth.h"

void user::auth::UserPasswordAuthentification::reset_defaults() {
  db.clear();
  #ifdef DEFAULT_ADMIN_PASSWORD
  db[admin] = DEFAULT_ADMIN_PASSWORD;
  #else
  db[admin] = "no-default-set";
  #endif
}

void user::auth::UserPasswordAuthentification::read_from_json(JsonObjectConst serialized_conf) {
  db.clear();
  for (JsonPairConst kv : serialized_conf) {
    db[kv.key().c_str()] = kv.value().as<std::string>();
  }
}

void user::auth::UserPasswordAuthentification::write_to_json(JsonObject target) const {
  for(auto const& kv : db) {
    target[kv.first] = kv.second;
  }
}

void user::auth::UserPasswordAuthentification::status(JsonObject target) {
  target["enabled"] = !is_disabled();
  auto users = target.createNestedArray("users");
  for(auto const &kv : db) users.add(kv.first);
  // don't tell the passwords!
}

int user::auth::UserPasswordAuthentification::login(JsonObjectConst msg_in, JsonObject &msg_out, user::auth::AuthentificationContext& user_context) {
  #ifdef ANABRID_UNSAFE_INTERNET
  msg_out["error"] = "No authentification neccessary. Auth system is disabled in this firmware build.";
  return 10;
  #else
  std::string new_user = msg_in["user"];
  if(user_context.hasBetterClearenceThen(new_user)) {
    // This case avoids users from locking themself out of the serial console where they
    // always have admin permissions. It is not that interesting in a TCP/IP connection.
    msg_out["error"] = "Login can only upgrade privileges but you wold loose. Open a new connection instead.";
    return 1;
  } else if(!is_valid(new_user, msg_in["password"])) {
    msg_out["error"] = "Invalid username or password.";

    // todo: besseren DEBUG flag für finden, vgl. https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/merge_requests/3#note_2361
    #if defined(ANABRID_DEBUG) || defined(ANABRID_DEBUG_INIT)
    // Show correct usernames and passwords on the console
    Serial.println("Since authentificiation failed and debug mode is on, this is the list of currently allowed passwords:");
    // TODO: Use LOG statements instead of Serial.print.
    StaticJsonDocument<500> kvmap;
    write_to_json(kvmap.to<JsonObject>());
    serializeJson(kvmap, Serial);
    Serial.println();
    #endif

    return 2;
  } else {
    user_context.login(new_user);

    // TODO: Somehow this line doesn't show up.
    Serial.print("New authentification: "); user_context.printTo(Serial); Serial.println();
    
    return 0;
  }
  #endif  
}

// The following snippet is here instead of in auth.h to avoid a circular depency

#include "user/settings.h"
user::auth::AuthentificationContext::AuthentificationContext() : auth(user::UserSettings.auth) {}
