#pragma once

#ifdef ARDUINO

#include "utils/logging.h"
#include "CrashReport.h" // core/teensy4
// #include "StreamUtils.h"

namespace utils {

    /**
     * Logs out what has been captured by the Teensy CrashReport tooling,
     * cf. https://www.pjrc.com/teensy/td_crashreport.html
     * 
     * This can help to debug faults generated by the memory protection unit (MPU)
     * such as null pointer references or other pointer arithmetics gone bad.
     * In order to have this work, the microcontroller needs to do a "soft-reboot"
     * which preserves the RAM content so the "post-mortem report" is available.
     * 
     * In order to understand the error codes, have a loook at the IMXRT1060
     * processor architecture manual, for instance at
     * https://www.pjrc.com/teensy/DDI0403Ee_arm_v7m_ref_manual.pdf
     * at page 611
     * 
     * 
     **/
    void check_and_log_crash() {
        if(CrashReport) {
            // StringPrint buf;
            // crash.printTo(buf);
            // buf.str();
            msg::Log::get().println("ALERT: Previous System crash detected. Post Mortem Report:");
            msg::Log::get().println(CrashReport);
        }
    }

} // ns
#else /* not arduino */

namespace utils {

} //ns

#endif /* ARDUINO */