#include "protocol/handler.h"
#include "daq/daq.h"

namespace msg {
namespace handlers {

class OneshotDAQHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      return error(daq::OneshotDAQ().sample(msg_in, msg_out));
  }
};


} // ns handlers
} // ns msg