#include "websockets/tcp.h"

#ifdef ARDUINO

#include "web/server.h"

using web::LucidacWebsocketsClient;

websockets::network::TcpClient::TcpClient(LucidacWebsocketsClient *context)
    : context(context), client(&context->socket) {}

#endif // ARDUINO
