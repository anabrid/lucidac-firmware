#include "protocol/handler.h"
#include "plugin/plugin.h"

namespace msg {

namespace handlers {

/// @ingroup MessageHandlers
class LoadPluginHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(loader::PluginLoader.load_and_execute(msg_in, msg_out));
    }
};

/// @ingroup MessageHandlers
class UnloadPluginHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    return error(loader::PluginLoader.unload(msg_in, msg_out));
    }
};


} // namespace handlers

} // namespace msg