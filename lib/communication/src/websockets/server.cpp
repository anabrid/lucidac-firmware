#include "websockets/server.h"

#ifdef ARDUINO

#include "web/websockets.h"

#include <map>
#include <memory>

namespace websockets {
WebsocketsServer::WebsocketsServer(network::TcpServer *server) : _server(server) {}

bool WebsocketsServer::available() { return this->_server->available(); }

void WebsocketsServer::listen(uint16_t port) { this->_server->listen(port); }

FLASHMEM
bool WebsocketsServer::poll() { return this->_server->poll(); }

struct ParsedHandshakeParams {
  std::string head;
  std::map<std::string, std::string> headers;
};

FLASHMEM
ParsedHandshakeParams recvHandshakeRequest(network::TcpClient &client) {
  ParsedHandshakeParams result;

  result.head = client.readLine();

  std::string line = client.readLine();
  do {
    std::string key = "", value = "";
    size_t idx = 0;

    // read key
    while (idx < line.size() && line[idx] != ':') {
      key += line[idx];
      idx++;
    }

    // skip key and whitespace
    idx++;
    while (idx < line.size() && (line[idx] == ' ' || line[idx] == '\t'))
      idx++;

    // read value (until \r\n)
    while (idx < line.size() && line[idx] != '\r') {
      value += line[idx];
      idx++;
    }

    // store header
    result.headers[key] = value;

    line = client.readLine();
  } while (client.available() && line != "\r\n");

  return result;
}

FLASHMEM
WebsocketsClient WebsocketsServer::accept() {
  std::shared_ptr<network::TcpClient> tcpClient(_server->accept());
  if (tcpClient->available() == false)
    return {};

  auto params = recvHandshakeRequest(*tcpClient);

  if (params.headers["Connection"].find("Upgrade") == std::string::npos)
    return {};
  if (params.headers["Upgrade"] != "websocket")
    return {};
  if (params.headers["Sec-WebSocket-Version"] != "13")
    return {};
  if (params.headers["Sec-WebSocket-Key"] == "")
    return {};

  // FIXME link to existing implementation if we want this

  std::string serverAccept = "fixme"; // crypto::websocketsHandshakeEncodeKey(
                                      // params.headers["Sec-WebSocket-Key"]
  //);

  tcpClient->send("HTTP/1.1 101 Switching Protocols\r\n");
  tcpClient->send("Connection: Upgrade\r\n");
  tcpClient->send("Upgrade: websocket\r\n");
  tcpClient->send("Sec-WebSocket-Version: 13\r\n");
  tcpClient->send("Sec-WebSocket-Accept: " + serverAccept + "\r\n");
  tcpClient->send("\r\n");

  WebsocketsClient wsClient(tcpClient);
  // Don't use masking from server to client (according to RFC)
  wsClient.setUseMasking(false);
  return wsClient;
}

WebsocketsServer::~WebsocketsServer() { this->_server->close(); }

} // namespace websockets

#endif // ARDUINO
