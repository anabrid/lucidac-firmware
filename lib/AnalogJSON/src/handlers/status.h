#include "message_handlers.h"

#include "user/user_auth.h"

namespace msg {
namespace handlers {

class GetSystemStatus : public MessageHandler {
  user::auth::UserPasswordAuthentification& _auth;
public:
  GetSystemStatus(user::auth::UserPasswordAuthentification& auth) : _auth(auth) {}
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override;
};


} // namespace handlers

} // namespace msg