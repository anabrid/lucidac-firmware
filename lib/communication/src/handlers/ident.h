#include "protocol/handler.h"
#include "utils/hashflash.h"
#include "build/distributor.h"
#include "nvmconfig/vendor.h"

namespace msg {
namespace handlers {

/// @ingroup MessageHandlers
class GetSystemIdent : public MessageHandler {
public:
  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
      msg_out["serial"] = nvmconfig::VendorOTP().serial_number;

      dist::write_to_json(msg_out["fw_build"]);
      loader::flashimage::toJson(msg_out["fw_image"]);
      return success;
  }
};


} // ns handlers
} // ns msg