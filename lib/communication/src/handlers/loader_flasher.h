#include "protocol/handler.h"
#include "ota/flasher.h"

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class FlasherInitHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override { return loader::FirmwareFlasher::get().init(msg_in, msg_out); }
};

/// @ingroup MessageHandlers
class FlasherDataHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  { return loader::FirmwareFlasher::get().stream(msg_in, msg_out); }
};

/// @ingroup MessageHandlers
class FlasherAbortHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  { return loader::FirmwareFlasher::get().abort(msg_in, msg_out); }
};

/// @ingroup MessageHandlers
class FlasherCompleteHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  { return loader::FirmwareFlasher::get().complete(msg_in, msg_out); }
};

} // namespace handlers

} // namespace msg