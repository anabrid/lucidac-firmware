#include "user/settings.h"
#include "web/server.h"
#include "utils/logging.h"
#include "protocol/protocol.h"
#include "build/distributor.h"

#define ENABLE_AWOT_NAMESPACE
#include <aWOT.h>

using namespace web;

awot::Application webapp;

LucidacWebServer &web::LucidacWebServer::get() {
  static LucidacWebServer obj;
  return obj;
}

void index(awot::Request& req, awot::Response& res) {
  res.status(200);
  res.set("Content-Type", "text/html");
  res.println("Hello World!");
}

// Test successful: File looks fine in browser.
// TODO: Checkout caching https://developer.mozilla.org/en-US/docs/Web/HTTP/Caching
//
// Note: Could make headers part of the files, including Last-Modified
// and etag, something as in:
/*
Content-Type: application/javascript
Content-Length: 1024
Cache-Control: public, max-age=31536000
Last-Modified: Tue, 22 Feb 2024 20:20:20 GMT
ETag: YsAIAAAA-QG4G6kCMAMBAAAAAAAoK

contents of file goes here...
*/
void testfile(awot::Request& req, awot::Response& res) {
  #if 0
  // vim test.txt; gzip test.txt; xxi -i test.txt.gz > test.h
  #include "test.h"

  res.set("Server", "LucidacWebServer/" FIRMWARE_VERSION);
  res.set("Content-Encoding", "gzip");
  res.set("Content-Length", String(test_txt_gz_len).c_str());
  res.set("Content-Type", "text/plain");
  res.write((char*)test_txt_gz, test_txt_gz_len);
  #else
  res.println("code disabled");
  #endif
}

void api_preflight(awot::Request& req, awot::Response& res, bool set_status=true) {
  res.set("Server", "LucidacWebServer/" FIRMWARE_VERSION);
  res.set("Content-Type", "application/json");
  // Disable CORS for the time being
  // Note the maximum number of headers of the lib (can be avoided by directly printing)
  res.set("Access-Control-Allow-Origin", "*");
  res.set("Access-Control-Allow-Credentials", "true");
  res.set("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  res.set("Access-Control-Allow-Headers", "Origin, Cookie, Set-Cookie, Content-Type, Server");  
  if(set_status)
    res.status(200);
}

void api(awot::Request& req, awot::Response& res) {
  api_preflight(req, res, /* status*/ false);

  // the following mimics the function
  // msg::JsonLinesProtocol::get().process_tcp_input(req, user::auth::AuthentificationContext());
  // which does not work because it assumes input stream = output sream.

  auto error = deserializeJson(*msg::JsonLinesProtocol().get().envelope_in, req);
  if (error == DeserializationError::Code::EmptyInput) {
    res.status(400);
    res.println("{'type':'error', 'msg':'Empty input. Expecting a Lucidac query in JSON fomat'}");
  } else if (error) {
    res.status(400);
    res.println("{'type':'error', 'msg': 'Error parsing JSON'}");
    //Serial.println(error.c_str());
  } else {
    res.status(200);
    static user::auth::AuthentificationContext c;
    msg::JsonLinesProtocol().get().handleMessage(c);
    serializeJson(msg::JsonLinesProtocol().get().envelope_out->as<JsonObject>(), Serial);
    serializeJson(msg::JsonLinesProtocol().get().envelope_out->as<JsonObject>(), res);
  }
}

void web::LucidacWebServer::begin() {
  ethserver.begin(80);
  webapp.get("/", &index);
  webapp.get("/api", &api);
  webapp.post("/api", &api);
  webapp.options("/api", &api_preflight);
  webapp.get("/test.txt", &testfile);
}

void web::LucidacWebServer::loop() {
  net::EthernetClient client_socket = ethserver.accept();

  // Warning: This probably allows also to deadlock the device by opening a connection
  //          but not sending anything.

  if(client_socket.connected()) {
    LOG4("Web Client request from ", client_socket.remoteIP(), ":", client_socket.remotePort());
    webapp.process(&client_socket);
  }

  client_socket.close(); // neccessary, otherwise browser wait for more data

  // the following "keep alive" code does not properly work
  /*
  if(client_socket) {
    // This code is as ugly as the original in protocol/protocol.h
    Client client;
    client.socket = client_socket;

    // note there is also net::EthernetClient::maxSockets(), which is dominated by basic system constraints.
    // This limit here is rather dominated by application level constraints and can probably be a counter
    // measure against ddos attacks.
    if(clients.size() == user::UserSettings.ethernet.max_connections) {
      LOG5("Cannot accept web client from ", client_socket.remoteIP(), " because maximum number of connections (",
        user::UserSettings.ethernet.max_connections, ") already reached.");
      client_socket.stop();
    } else {
      clients.push_back(client);

      // HTTP/1 has no push. Can only do that on websockets.
      // Or need a query'able buffer instead for basic state changes, for instance.
      // JsonLinesProtocol::get().broadcast.add(&(std::prev(clients.end())->socket));

      LOG4("Web Client ", clients.size()-1, " connected from ", client_socket.remoteIP());
    }
  }

  // using iterator instead range-based loop because EthernetClient lacks == operator
  // so we use list::erase instead of list::remove.
  for(auto client = clients.begin(); client != clients.end(); client++) {
    const auto client_idx = std::distance(clients.begin(), client);
    if(client->socket.connected()) {
      if(client->socket.available() > 0) {
        //msg::JsonLinesProtocol::get().process_tcp_input(client->socket, client->user_context);
        webapp.process(&client->socket);
        client->last_contact.reset();
      } else if(!client->socket.connected()) {
        client->socket.stop();
      } else if(client->last_contact.expired(user::UserSettings.ethernet.connection_timeout_ms)) {
        LOG5("Web client ", client_idx, ", timed out after ", user::UserSettings.ethernet.connection_timeout_ms, " ms of idling");
        client->socket.stop();
      }
    } else {
      LOG5("Web Client ", client_idx, ", was ", client->socket.remoteIP(), ", disconnected");
      client->socket.close();
      //JsonLinesProtocol::get().broadcast.remove(&client->socket);
      clients.erase(client);
      return; // iterator invalidated, better start loop() freshly.
    }
  }
  */
}
