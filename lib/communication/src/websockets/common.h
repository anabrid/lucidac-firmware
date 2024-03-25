#pragma once

#include <string>


// WS Client Config
#define _WS_BUFFER_SIZE 1024
//#define _WS_CONFIG_NO_TRUE_RANDOMNESS
//#define _WS_CONFIG_SKIP_HANDSHAKE_ACCEPT_VALIDATION
//#define _WS_CONFIG_MAX_MESSAGE_SIZE 1000
//#define _WS_CONFIG_NO_SSL

#define PLATFORM_DOES_NOT_SUPPORT_BLOCKING_READ

#include "tcp.h"

namespace websockets {
    namespace internals {
        inline std::string fromInterfaceString(const std::string& str) { return str; }
        inline std::string fromInterfaceString(const std::string&& str) { return str; }
        inline std::string fromInternalString(const std::string& str) { return str; }
        inline std::string fromInternalString(const std::string&& str) { return str; }
    }
}
