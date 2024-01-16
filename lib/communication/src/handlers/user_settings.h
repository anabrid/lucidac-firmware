#pragma once

#include "user/settings.h"
#include "protocol/handler.h"

namespace msg {

namespace handlers {


class GetSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    user::UserSettings.write_to_json(msg_out);
    return success;
}
};

class SetSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    user::UserSettings.set(msg_in,msg_out);
    return success;
}
};

class ResetSettingsHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    user::UserSettings.reset_defaults();
    return success;
}
};

} // namespace handlers

} // namespace msg