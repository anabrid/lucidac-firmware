#include "protocol/handler.h"

#include "mode/mode.h"
#include "run/run_manager.h"

namespace msg {

namespace handlers {

/**
 * Message handler that allows manual IC/OP/HALT state control.
 * 
 * Basic protection against messing with the RunManager and FlexIOControl is given.
 * That means, it will only work if the LUCIDAC is in "idle mode".
 * 
 * @warning Known limitations:
 *   Manual control is primarily useful for debugging.
 *   It won't trigger any DAQ aquisition. 
 * 
 * @ingroup MessageHandlers
 **/
class ManualControlHandler : public MessageHandler {

public:
  /// @TODO This should be done in mode/mode.h or run/run.h instead.
  ///       Returns 0 if can run.
  static int can_manual_control() {
    if(!run::RunManager::get().queue.empty()) return 1;
    if(!mode::FlexIOControl::is_done()) return 2;
    // maybe further checks neccessary.
    return 0;
  }

  int handle(JsonObjectConst msg_in, JsonObject &msg_out) override {
    int res_run_and_mode = can_manual_control();
    if(res_run_and_mode) return error(res_run_and_mode);

    if(msg_in["to"] == "ic")        mode::ManualControl::to_ic();
    else if(msg_in["to"] == "op")   mode::ManualControl::to_op();
    else if(msg_in["to"] == "halt") mode::ManualControl::to_halt();
    else return error(10); // illegal target state

    return success;
  }
};

} // namespace handlers

} // namespace msg