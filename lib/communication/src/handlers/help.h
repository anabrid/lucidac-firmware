#include "protocol/handler.h"

#include "protocol/registry.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class HelpHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    msg_out["human_readable_info"] = "This is a JSON-Lines protocol described at https://anabrid.dev/docs/hybrid-controller/";

    auto types_list = msg_out.createNestedArray("available_types");
    msg::handlers::Registry.write_handler_names_to(types_list);

    return success;
  }
};



} // namespace handlers

} // namespace msg