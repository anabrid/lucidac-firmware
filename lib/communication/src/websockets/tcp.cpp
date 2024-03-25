#include "websockets/tcp.h"
#include "web/server.h"

using web::LucidacWebsocketsClient;

websockets::network::TcpClient::TcpClient(LucidacWebsocketsClient* context) : context(context), client(&context->socket) {}
