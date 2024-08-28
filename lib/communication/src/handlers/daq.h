#include "protocol/handler.h"
#include "daq/daq.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class OneshotDAQHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      daq::OneshotDAQ().init(0);
      return error(daq::OneshotDAQ().sample(msg_in, msg_out));
  }
};


} // ns handlers
} // ns msg