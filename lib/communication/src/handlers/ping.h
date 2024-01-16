#include "protocol/handler.h"

namespace msg {
namespace handlers {


class PingRequestHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  {
    msg_out["now"] = "2007-08-31T16:47+01:00";
    // Note, with some initial NTP call we could get micro-second time resolution if we need it
    // for whatever reason.
    msg_out["micros"] = micros();
    return success;
  }
};


} // namespace handlers

} // namespace msg