#include "protocol/jsonl_server.h"

#ifdef ARDUINO

#include "net/ethernet.h"
#include "protocol/protocol.h"
#include "utils/logging.h"

void msg::JsonlServer::begin() { server.begin(net::StartupConfig::get().jsonl_port); }

void msg::JsonlServer::loop() {
  net::EthernetClient client_socket = server.accept();

  if (client_socket) {
    // note that the following code copies "socket" object multiple times. When using pointers into
    // the struct, do not use the pointer to the wrong version.
    // TODO: Clean up code.
    Client client;
    client.socket = client_socket;
    client.user_context.set_remote_identifier(net::auth::RemoteIdentifier{client_socket.remoteIP()});

    // note there is also net::EthernetClient::maxSockets(), which is dominated by basic system constraints.
    // This limit here is rather dominated by application level constraints and can probably be a counter
    // measure against ddos attacks.
    if (clients.size() == net::StartupConfig::get().max_connections) {
      LOG5("Cannot accept client from ", client_socket.remoteIP(), " because maximum number of connections (",
           net::StartupConfig::get().max_connections, ") already reached.");
      client_socket.stop();
    } else {
      clients.push_back(client);
      msg::JsonLinesProtocol::get().broadcast.add(&(std::prev(clients.end())->socket));
      LOG4("Client ", clients.size() - 1, " connected from ", client_socket.remoteIP());
    }
  }

  // using iterator instead range-based loop because EthernetClient lacks == operator
  // so we use list::erase instead of list::remove.
  for (auto client = clients.begin(); client != clients.end(); client++) {
    const auto client_idx = std::distance(clients.begin(), client);
    if (client->socket.connected()) {
      if (client->socket.available() > 0) {
        msg::JsonLinesProtocol::get().process_tcp_input(client->socket, client->user_context);
        client->last_contact.reset();
      } else if (client->last_contact.expired(net::StartupConfig::get().connection_timeout_ms)) {
        LOG5("Client ", client_idx, ", timed out after ", net::StartupConfig::get().connection_timeout_ms,
             " ms of idling");
        client->socket.stop();
      }
    } else {
      LOG5("Client ", client_idx, ", was ", client->user_context, ", disconnected");
      msg::JsonLinesProtocol::get().broadcast.remove(&client->socket);
      client->socket.stop(); // important, stop waits
      clients.erase(client);
      return; // iterator invalidated, better start loop() freshly.
    }
  }
}

#endif // ARDUINO
