#include "protocol/handler.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class RebootHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  {
    loader::reboot(); // does actually not return
    return success;
  }
};


} // namespace handlers
} // namespace msg