#pragma once

#include <string>


namespace web {

    /**
     * Compute the websocket handshake key stuff,
     * cf. https://developer.mozilla.org/en-US/docs/Web/HTTP/Headers/Sec-WebSocket-Accept
     **/
    std::string websocketsHandshakeEncodeKey(const char* key);
}