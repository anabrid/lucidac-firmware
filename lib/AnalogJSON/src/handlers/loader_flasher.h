#include "message_handlers.h"
#include "loader/flasher.h"

namespace msg {

namespace handlers {


class FlasherInitHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override { loader::FirmwareFlasher::get().init(msg_in, msg_out); }
};

class FlasherDataHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  { loader::FirmwareFlasher::get().handle(msg_in, msg_out); }
};

class FlasherAbortHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  { loader::FirmwareFlasher::get().abort(msg_in, msg_out); }
};

class FlasherCompleteHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  { loader::FirmwareFlasher::get().complete(msg_in, msg_out); }
};

} // namespace handlers

} // namespace msg