#pragma once

#include <Arduino.h>
#ifdef ARDUINO

#include "websockets/common.h"
#include "websockets/data_frame.h"
#include "websockets/endpoint.h"
#include "websockets/message.h"
#include "websockets/tcp.h"

#include <functional>
#include <memory>
#include <vector>

namespace websockets {
enum class WebsocketsEvent { ConnectionOpened, ConnectionClosed, GotPing, GotPong };

class WebsocketsClient;
typedef std::function<void(WebsocketsClient &, WebsocketsMessage)> MessageCallback;
typedef std::function<void(WebsocketsMessage)> PartialMessageCallback;

typedef std::function<void(WebsocketsClient &, WebsocketsEvent, std::string)> EventCallback;
typedef std::function<void(WebsocketsEvent, std::string)> PartialEventCallback;

class WebsocketsClient {
public:
  WebsocketsClient();
  WebsocketsClient(std::shared_ptr<network::TcpClient> client);

  WebsocketsClient(const WebsocketsClient &other);
  WebsocketsClient(const WebsocketsClient &&other);

  WebsocketsClient &operator=(const WebsocketsClient &other);
  WebsocketsClient &operator=(const WebsocketsClient &&other);

  /// @deprecated comment out, only for connecting.
  void addHeader(const std::string key, const std::string value);

  // bool connect(const std::string url);
  // bool connect(const std::string host, const int port, const std::string path);

  void onMessage(const MessageCallback callback);
  void onMessage(const PartialMessageCallback callback);

  void onEvent(const EventCallback callback);
  void onEvent(const PartialEventCallback callback);

  bool poll();
  bool available(const bool activeTest = false);

  bool send(const std::string &&data);
  bool send(const std::string &data);
  bool send(const char *data);
  bool send(const char *data, const size_t len);

  bool sendBinary(const std::string data);
  bool sendBinary(const char *data, const size_t len);

  // stream messages
  bool stream(const std::string data = "");
  bool streamBinary(const std::string data = "");
  bool end(const std::string data = "");

  void setFragmentsPolicy(const FragmentsPolicy newPolicy);
  FragmentsPolicy getFragmentsPolicy() const;

  WebsocketsMessage readBlocking();

  bool ping(const std::string data = "");
  bool pong(const std::string data = "");

  void close(const CloseReason reason = CloseReason_NormalClosure);
  CloseReason getCloseReason() const;

  void setUseMasking(bool useMasking) { _endpoint.setUseMasking(useMasking); }

  virtual ~WebsocketsClient();

  std::shared_ptr<network::TcpClient> client() { return _client; }

private:
  std::shared_ptr<network::TcpClient> _client;
  std::vector<std::pair<std::string, std::string>> _customHeaders;
  internals::WebsocketsEndpoint _endpoint;
  bool _connectionOpen;
  MessageCallback _messagesCallback;
  EventCallback _eventsCallback;

  enum SendMode { SendMode_Normal, SendMode_Streaming } _sendMode;

  void _handlePing(WebsocketsMessage);
  void _handlePong(WebsocketsMessage);
  void _handleClose(WebsocketsMessage);

  // void upgradeToSecuredConnection();
};
} // namespace websockets

#endif // ARDUINO
