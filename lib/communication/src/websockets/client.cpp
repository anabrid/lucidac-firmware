#include "websockets/client.h"

#ifdef ARDUINO

#include "websockets/common.h"
#include "websockets/message.h"
#include "websockets/tcp.h"
#include <string.h>

namespace websockets {
WebsocketsClient::WebsocketsClient() : WebsocketsClient(std::make_shared<websockets::network::TcpClient>()) {
  // Empty
}

FLASHMEM
WebsocketsClient::WebsocketsClient(std::shared_ptr<network::TcpClient> client)
    : _client(client), _endpoint(client), _connectionOpen(client->available()),
      _messagesCallback([](WebsocketsClient &, WebsocketsMessage) {}),
      _eventsCallback([](WebsocketsClient &, WebsocketsEvent, std::string) {}), _sendMode(SendMode_Normal) {
  // Empty
}

FLASHMEM
WebsocketsClient::WebsocketsClient(const WebsocketsClient &other)
    : _client(other._client), _endpoint(other._endpoint), _connectionOpen(other._client->available()),
      _messagesCallback(other._messagesCallback), _eventsCallback(other._eventsCallback),
      _sendMode(other._sendMode) {

  // delete other's client
  const_cast<WebsocketsClient &>(other)._client = nullptr;
  const_cast<WebsocketsClient &>(other)._connectionOpen = false;
}

FLASHMEM
WebsocketsClient::WebsocketsClient(const WebsocketsClient &&other)
    : _client(other._client), _endpoint(other._endpoint), _connectionOpen(other._client->available()),
      _messagesCallback(other._messagesCallback), _eventsCallback(other._eventsCallback),
      _sendMode(other._sendMode) {

  // delete other's client
  const_cast<WebsocketsClient &>(other)._client = nullptr;
  const_cast<WebsocketsClient &>(other)._connectionOpen = false;
}

FLASHMEM
WebsocketsClient &WebsocketsClient::operator=(const WebsocketsClient &other) {
  // call endpoint's copy operator
  _endpoint = other._endpoint;

  // get callbacks and data from other
  this->_client = other._client;
  this->_messagesCallback = other._messagesCallback;
  this->_eventsCallback = other._eventsCallback;
  this->_connectionOpen = other._connectionOpen;
  this->_sendMode = other._sendMode;

  // delete other's client
  const_cast<WebsocketsClient &>(other)._client = nullptr;
  const_cast<WebsocketsClient &>(other)._connectionOpen = false;
  return *this;
}

FLASHMEM
WebsocketsClient &WebsocketsClient::operator=(const WebsocketsClient &&other) {
  // call endpoint's copy operator
  _endpoint = other._endpoint;

  // get callbacks and data from other
  this->_client = other._client;
  this->_messagesCallback = other._messagesCallback;
  this->_eventsCallback = other._eventsCallback;
  this->_connectionOpen = other._connectionOpen;
  this->_sendMode = other._sendMode;

  // delete other's client
  const_cast<WebsocketsClient &>(other)._client = nullptr;
  const_cast<WebsocketsClient &>(other)._connectionOpen = false;
  return *this;
}

FLASHMEM
bool isWhitespace(char ch) { return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n'; }

FLASHMEM
bool isCaseInsensetiveEqual(const std::string lhs, const std::string rhs) {
  if (lhs.size() != rhs.size())
    return false;

  for (size_t i = 0; i < lhs.size(); i++) {
    char leftLowerCaseChar = lhs[i] >= 'A' && lhs[i] <= 'Z' ? lhs[i] - 'A' + 'a' : lhs[i];
    char righerLowerCaseChar = rhs[i] >= 'A' && rhs[i] <= 'Z' ? rhs[i] - 'A' + 'a' : rhs[i];
    if (leftLowerCaseChar != righerLowerCaseChar)
      return false;
  }

  return true;
}

FLASHMEM
bool doestStartsWith(std::string str, std::string prefix) {
  if (str.size() < prefix.size())
    return false;
  for (size_t i = 0; i < prefix.size(); i++) {
    if (str[i] != prefix[i])
      return false;
  }

  return true;
}

FLASHMEM
void WebsocketsClient::addHeader(const std::string key, const std::string value) {
  _customHeaders.push_back({internals::fromInterfaceString(key), internals::fromInterfaceString(value)});
}

FLASHMEM
void WebsocketsClient::onMessage(MessageCallback callback) { this->_messagesCallback = callback; }

FLASHMEM
void WebsocketsClient::onMessage(PartialMessageCallback callback) {
  this->_messagesCallback = [callback](WebsocketsClient &, WebsocketsMessage msg) { callback(msg); };
}

FLASHMEM
void WebsocketsClient::onEvent(EventCallback callback) { this->_eventsCallback = callback; }

FLASHMEM
void WebsocketsClient::onEvent(PartialEventCallback callback) {
  this->_eventsCallback = [callback](WebsocketsClient &, WebsocketsEvent event, std::string data) {
    callback(event, data);
  };
}

FLASHMEM
bool WebsocketsClient::poll() {
  bool messageReceived = false;
  while (available() && _endpoint.poll()) {
    auto msg = _endpoint.recv();
    if (msg.isEmpty()) {
      continue;
    }
    messageReceived = true;

    if (msg.isBinary() || msg.isText()) {
      this->_messagesCallback(*this, std::move(msg));
    } else if (msg.isContinuation()) {
      // continuation messages will only be returned when policy is appropriate
      this->_messagesCallback(*this, std::move(msg));
    } else if (msg.isPing()) {
      _handlePing(std::move(msg));
    } else if (msg.isPong()) {
      _handlePong(std::move(msg));
    } else if (msg.isClose()) {
      this->_connectionOpen = false;
      _handleClose(std::move(msg));
    }
  }

  return messageReceived;
}

WebsocketsMessage WebsocketsClient::readBlocking() {
  while (available()) {
#ifdef PLATFORM_DOES_NOT_SUPPORT_BLOCKING_READ
    while (available() && _endpoint.poll() == false)
      continue;
#endif
    auto msg = _endpoint.recv();
    if (!msg.isEmpty())
      return msg;
  }
  return {};
}

bool WebsocketsClient::send(const std::string &data) {
  auto str = internals::fromInterfaceString(data);
  return this->send(str.c_str(), str.size());
}

bool WebsocketsClient::send(const std::string &&data) {
  auto str = internals::fromInterfaceString(data);
  return this->send(str.c_str(), str.size());
}

bool WebsocketsClient::send(const char *data) { return this->send(data, strlen(data)); }

FLASHMEM
bool WebsocketsClient::send(const char *data, const size_t len) {
  if (available()) {
    // if in normal mode
    if (this->_sendMode == SendMode_Normal) {
      // send a normal message
      return _endpoint.send(data, len, internals::ContentType::Text, true);
    }
    // if in streaming mode
    else if (this->_sendMode == SendMode_Streaming) {
      // send a continue frame
      return _endpoint.send(data, len, internals::ContentType::Continuation, false);
    }
  }
  return false;
}

bool WebsocketsClient::sendBinary(std::string data) {
  auto str = internals::fromInterfaceString(data);
  return this->sendBinary(str.c_str(), str.size());
}

FLASHMEM
bool WebsocketsClient::sendBinary(const char *data, const size_t len) {
  if (available()) {
    // if in normal mode
    if (this->_sendMode == SendMode_Normal) {
      // send a normal message
      return _endpoint.send(data, len, internals::ContentType::Binary, true);
    }
    // if in streaming mode
    else if (this->_sendMode == SendMode_Streaming) {
      // send a continue frame
      return _endpoint.send(data, len, internals::ContentType::Continuation, false);
    }
  }
  return false;
}

bool WebsocketsClient::stream(const std::string data) {
  if (available() && this->_sendMode == SendMode_Normal) {
    this->_sendMode = SendMode_Streaming;
    return _endpoint.send(internals::fromInterfaceString(data), internals::ContentType::Text, false);
  }
  return false;
}

bool WebsocketsClient::streamBinary(const std::string data) {
  if (available() && this->_sendMode == SendMode_Normal) {
    this->_sendMode = SendMode_Streaming;
    return _endpoint.send(internals::fromInterfaceString(data), internals::ContentType::Binary, false);
  }
  return false;
}

bool WebsocketsClient::end(const std::string data) {
  if (available() && this->_sendMode == SendMode_Streaming) {
    this->_sendMode = SendMode_Normal;
    return _endpoint.send(internals::fromInterfaceString(data), internals::ContentType::Continuation, true);
  }
  return false;
}

void WebsocketsClient::setFragmentsPolicy(const FragmentsPolicy newPolicy) {
  _endpoint.setFragmentsPolicy(newPolicy);
}

FLASHMEM
bool WebsocketsClient::available(const bool activeTest) {
  if (activeTest) {
    _endpoint.ping("");
  }

  bool updatedConnectionOpen = this->_connectionOpen && this->_client && this->_client->available();

  if (updatedConnectionOpen != this->_connectionOpen) {
    _endpoint.close(CloseReason_AbnormalClosure);
    this->_eventsCallback(*this, WebsocketsEvent::ConnectionClosed, "");
  }

  this->_connectionOpen = updatedConnectionOpen;
  return this->_connectionOpen;
}

bool WebsocketsClient::ping(const std::string data) {
  if (available()) {
    return _endpoint.ping(internals::fromInterfaceString(data));
  }
  return false;
}

bool WebsocketsClient::pong(const std::string data) {
  if (available()) {
    return _endpoint.pong(internals::fromInterfaceString(data));
  }
  return false;
}

void WebsocketsClient::close(const CloseReason reason) {
  if (available()) {
    this->_connectionOpen = false;
    _endpoint.close(reason);
    _handleClose({});
  }
}

CloseReason WebsocketsClient::getCloseReason() const { return _endpoint.getCloseReason(); }

void WebsocketsClient::_handlePing(const WebsocketsMessage message) {
  this->_eventsCallback(*this, WebsocketsEvent::GotPing, message.data());
}

void WebsocketsClient::_handlePong(const WebsocketsMessage message) {
  this->_eventsCallback(*this, WebsocketsEvent::GotPong, message.data());
}

void WebsocketsClient::_handleClose(const WebsocketsMessage message) {
  this->_eventsCallback(*this, WebsocketsEvent::ConnectionClosed, message.data());
}

WebsocketsClient::~WebsocketsClient() {
  if (available()) {
    this->close(CloseReason_GoingAway);
  }
}
} // namespace websockets

#endif // ARDUINO
