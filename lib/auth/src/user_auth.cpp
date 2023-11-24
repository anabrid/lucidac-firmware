#include "user_auth.h"
#include "user_login.h"
#include "distributor.h"

void auth::UserPasswordAuthentification::reset_defaults() {
  db.clear();
  db[admin] = DEFAULT_ADMIN_PASSWORD;
}

void auth::UserPasswordAuthentification::read_from_json(JsonObjectConst serialized_conf) {
  db.clear();
  for (JsonPairConst kv : serialized_conf) {
    db[kv.key().c_str()] = kv.value().as<std::string>();
  }
}

void auth::UserPasswordAuthentification::write_to_json(JsonObject target) {
  for(auto const& kv : db) {
    target[kv.first] = kv.second;
  }
}

void auth::UserPasswordAuthentification::status(JsonObject target) {
  target["enabled"] = !is_disabled();
  auto users = target.createNestedArray("users");
  for(auto const &kv : db) users.add(kv.first);
  // don't tell the passwords!
}

bool msg::handlers::LoginHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out, auth::AuthentificationContext& user_context) {
  #ifdef ANABRID_UNSAFE_INTERNET
  msg_out["error"] = "No authentification neccessary. Auth system is disabled in this firmware build.";
  return false;
  #else
  std::string new_user = msg_in["user"];
  if(user_context.hasBetterClearenceThen(new_user)) {
    // This case avoids users from locking themself out of the serial console where they
    // always have admin permissions. It is not that interesting in a TCP/IP connection.
    msg_out["error"] = "Login can only upgrade privileges but you wold loose. Open a new connection instead.";
    return false;
  } else if(!auth.is_valid(new_user, msg_in["password"])) {
    msg_out["error"] = "Invalid username or password.";

    // todo: besseren DEBUG flag für finden, vgl. https://lab.analogparadigm.com/lucidac/firmware/hybrid-controller/-/merge_requests/3#note_2361
    #if defined(ANABRID_DEBUG) || defined(ANABRID_DEBUG_INIT)
    // Show correct usernames and passwords on the console
    Serial.println("Since authentificiation failed and debug mode is on, this is the list of currently allowed passwords:");
    // TODO: Use LOG statements instead of Serial.print.
    StaticJsonDocument<500> kvmap;
    auth.write_to_json(kvmap.to<JsonObject>());
    serializeJson(kvmap, Serial);
    Serial.println();
    #endif

    return false;
  } else {
    user_context.login(new_user);

    // TODO: Somehow this line doesn't show up.
    Serial.print("New authentification: "); user_context.printTo(Serial); Serial.println();
    
    return true;
  }
  #endif  
}
