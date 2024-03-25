#pragma once

#include "websockets/common.h"
#include "QNEthernet.h"

// Forward declaration to avoid circular reference
namespace web { class LucidacWebsocketsClient; }

namespace websockets { namespace network {
  using qindesign::network::EthernetServer;
  using qindesign::network::EthernetClient;
  using web::LucidacWebsocketsClient;

  struct TcpClient {
    EthernetClient* client;
    LucidacWebsocketsClient* context;

    // Formerly, this was a kind-of-socket
    // TcpClient(EthernetClient* client) : client(client) {}
    // But now we have the full client context
    TcpClient(LucidacWebsocketsClient* context);
    TcpClient() {}

    bool connect(std::string host, int port) {
      return client->connect(host.c_str(), port);
    }

    bool poll() {
      return client->available() > 0; // Returns the amount of data available, whether the connection is closed or not.
    }

    bool available()  {
      return (bool)client; // Returns whether connected (at least in QNEthernet).
    }

    void send(std::string data)  {
      client->writeFully(reinterpret_cast<uint8_t*>(const_cast<char*>(data.c_str())), data.size());
    }

    void send(uint8_t* data, uint32_t len)  {
      client->writeFully(data, len);
    }
    
    std::string readLine()  {
      int val;
      std::string line;
      do {
        val = client->read();
        if(val < 0) continue;
        line += (char)val;
      } while(val != '\n');
      if(!available()) close();
      return line;
    }

    int read(uint8_t* buffer, uint32_t len)  {
      return client->read(buffer, len);
    }

    void close()  {
      client->stop();
    }

    ~TcpClient() {
      client->stop();
    }
  };

  #define DUMMY_PORT 0

  class TcpServer {
    EthernetServer server;
  public:
    TcpServer() : server(DUMMY_PORT) {}
    bool poll()  {
      return server.availableForWrite();
    }

    bool listen(uint16_t port)  {
      server.begin(port);
      return available();
    }
    
    TcpClient* accept()  {
      while(available()) {
        auto client = server.available();
        // FIXME if we want to use the Server.
        // if(client) return new TcpClient(client);
      }
      return new TcpClient;
    }

    bool available()  {
      return server.availableForWrite();
    }
    
    void close()  {
      server.end();
    }

    ~TcpServer() {
      if(available()) close();
    }
  };

}} // websockets::network