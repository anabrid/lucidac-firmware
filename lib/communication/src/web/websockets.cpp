
// Code from https://github.com/gilmaimon/TinyWebsockets
#include "web/websockets.h"

#include <Arduino.h>
#include "utils/logging.h"

#include "utils/dcp.h"
#include "utils/etl_base64.h"

/*

  This can be manually unit tested with these two example fixtures

     websocketsHandshakeEncodeKey("dGhlIHNhbXBsZSBub25jZQ==") == "s3pPLMBiTxaQ9kYGzzhZRbK+xOo="
     websocketsHandshakeEncodeKey("x3JJHMbDL1EzLkh9GBhXDw==") == "HSmrc0sMlYUkAGmm5OPpG2HaGWk=");

  i.e. with curl:

      curl --verbose -H "Upgrade: websocket" -H "Connection: Upgrade" -H "Sec-Websocket-Version: 13"  -H "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==" http://lucidac-17-40-f4.fritz.box/websocket

*/
std::string web::websocketsHandshakeEncodeKey(const char* key) {
    if(!key) return;

    std::string input(key);
    input += "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    utils::sha1 sha1((const uint8_t*) input.c_str(), input.size());
    LOGMEV("sha1('%s') = '%s'\n", key, sha1.to_string());

    char retkey[30+1] = { 0 };
    etl::base64::encode(sha1.checksum.data(), 20, retkey, 30);

    LOGMEV("base64encode('%s') = '%s'\n", sha1.to_string(), retkey);

    return retkey;
}

