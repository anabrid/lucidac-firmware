#include "protocol/handler.h"
#include "utils/hashflash.h"
#include "build/distributor.h"
#include "nvmconfig/vendor.h"
#include "ota/flasher.h" // reboot()
#include "protocol/jsonl_logging.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class GetSystemIdent : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      msg_out["serial"] = nvmconfig::VendorOTP().serial_number;

      dist::write_to_json(msg_out.createNestedObject("fw_build"));
      loader::flashimage::toJson(msg_out.createNestedObject("fw_image"));
      return success;
  }
};

/// @ingroup MessageHandlers
class RebootHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override  {
    loader::reboot(); // does actually not return
    return success;
  }
};

/// @ingroup MessageHandlers
class SyslogHandler : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, utils::StreamingJson& msg_out) override  {
    msg::StartupLog::get().stream_to_json(msg_out);
    return success;
  }
};


} // ns handlers
} // ns msg