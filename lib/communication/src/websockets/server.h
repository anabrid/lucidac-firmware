#pragma once

#include "common.h"
#include "client.h"


#include <functional>

namespace websockets {
  class WebsocketsServer {
  public:
    WebsocketsServer(network::TcpServer* server = new network::TcpServer);
    
    WebsocketsServer(const WebsocketsServer& other) = delete;
    WebsocketsServer(const WebsocketsServer&& other) = delete;
    
    WebsocketsServer& operator=(const WebsocketsServer& other) = delete;
    WebsocketsServer& operator=(const WebsocketsServer&& other) = delete;

    bool available();
    void listen(uint16_t port);
    bool poll();
    WebsocketsClient accept();

    virtual ~WebsocketsServer();

  private:
    network::TcpServer* _server;
  };
}