#include "user_auth.h"
#include "distributor.h"

void auth::UserPasswordAuthentification::reset_defaults() {
  dist::Distributor dist;
  db.clear();
  db[admin] = dist.default_admin_password().c_str();
}

void auth::UserPasswordAuthentification::read_from_json(JsonObjectConst serialized_conf) {
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
  target["enabled"] = !isdisabled();
  auto users = target.createNestedArray("users");
  for(auto const &kv : db) users.add(kv.first);
  // don't tell the passwords!
}

bool msg::handlers::LoginHandler::handle(JsonObjectConst msg_in, JsonObject &msg_out, auth::AuthentificationContext& user_context) {
  std::string new_user = msg_in["user"];
  if(user_context.hasBetterClearenceThen(new_user)) {
    // This case avoids users from locking themself out of the serial console where they
    // always have admin permissions. It is not that interesting in a TCP/IP connection.
    msg_out["error"] = "Login can only upgrade privileges but you wold loose. Open a new connection instead.";
    return false;
  } else if(!auth.isvalid(new_user, msg_in["password"])) {
    msg_out["error"] = "Invalid username or password.";
    return false;
  } else {
    user_context.login(new_user);
    return true;
  }  
}
