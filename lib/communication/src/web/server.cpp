#include "user/settings.h"
#include "web/server.h"
#include "web/assets.h"
#include "utils/logging.h"
#include "protocol/protocol.h"
#include "build/distributor.h"

#define ENABLE_AWOT_NAMESPACE
#include <aWOT.h>

using namespace web;

#define SERVER_VERSION  "LucidacWebServer/" FIRMWARE_VERSION

awot::Application webapp;

LucidacWebServer &web::LucidacWebServer::get() {
  static LucidacWebServer obj;
  return obj;
}

void notfound(awot::Request& req, awot::Response& res) {
  res.status(404);
  res.set("Server", SERVER_VERSION);
  res.set("Content-Type", "text/html");
  res.println("<h1>Not found</h1><p>The ressource:<pre>");
  res.println(req.path());
  res.println("</pre><p>was not found on this server.");
  res.println("<hr><p><i>" SERVER_VERSION "</i>");
}

void serve_static(const web::StaticFile& file, awot::Response& res) {
  LOGMEV("Serving static file %s with %d bytes\n", file.filename, file.size);
  res.status(200);
  // copying the headers (dumb!)
  for(int i=0; i < file.max_headers; i++) {
    if(file.headers[i].name)
      res.set(file.headers[i].name, file.headers[i].value);
  }
  // this would have been smarter but aWOT is not very smart
  // res.print(file.prefab_http_headers);
  // res.endHeaders();
  for(
    uint32_t written = 0;
    written != file.size;
  ) {
    // Severe BUG in aWOT: v3.5.0 is lying about the result of write, it never passes the
    //    real write result. Therefore, attemping to full write at this API level does not
    //    work. An easy fix is sketched in
    //    https://github.com/lasselukkari/aWOT/compare/master...lfarrand:aWOT:master
    //    but using a Github fork is not easy as it has to be registered at PlatformIO.
    //    Suggest instead hosting the aWOT locally here in the firmware.
    //
    // I repeat: This line does NOT work for files larger then ~ 5kB.
    // Solution is obvious but has to be done at some point.
    uint32_t singleshot = res.write((uint8_t*)file.start+written, file.size - written);
    //LOGMEV("Written %d, plus %d", written, singleshot);
    written += singleshot;
  }
  res.flush();
}

void serve_static(awot::Request& req, awot::Response& res) {
  // as a little workaround for aWOT, determine whether this runs *after*
  // some successful route by inspecting whether something has been sent out or not.
  // Handle only if nothing has been sent so far, i.e. no previous middleware/endpoint
  // replied to this http query.
  if(res.bytesSent()) return;

  LOGMEV("%s", req.path());
  const char *path_without_leading_slash = req.path() + 1;
  const StaticFile* file = StaticAttic().get_by_filename(path_without_leading_slash);
  if(file) {
    serve_static(*file, res);
    res.end();
  } else {
    LOGMEV("No suitable static file found for path %s\n", req.path());
    notfound(req, res);
  }
}

void index(awot::Request& req, awot::Response& res) {
  const StaticFile* file = StaticAttic().get_by_filename("index.html");
  if(file) {
    res.status(200);
    serve_static(*file, res);
  } else {
    res.status(501);
    res.set("Content-Type", "text/html");
    res.println(
      "<h1>LUCIDAC: Welcome</h1>"
      "<p>Your LUCIDAC can reply to messages on the web via an elevated JSONL API endpoint. "
      "However. the Single Page Application (SPA) static files have not been installed on the firmware. "
      "This means you cannot use the beautiful and easy web interface straight out of the way. However, "
      "you can still use it if you have sufficiently permissive CORS settings active and use a public "
      "hosted version of the LUCIDAC GUI (SPA)."
      "<hr><p><i>" SERVER_VERSION "</i>"
    );
  }
}

namespace web {
void convertToJson(const StaticFile& file, JsonVariant serialized) {
  auto j = serialized.to<JsonObject>();
  j["filename"] = file.filename;
  j["lastmod"] = file.lastmod;
  j["size"] = (int)file.size;
  j["start_in_memory"] = (int)file.start;
}
}

// Provide some information about the built in assets
// TODO: Should actually integrate information about webserver status
// in the general {'type':'status'} query.
void about_static(awot::Request& req, awot::Response& res) {
  res.set("Server", SERVER_VERSION);
  res.set("Content-Type", "application/json");

  StaticAttic attic;

  StaticJsonDocument<1024> j;
  j["has_any_static_files"] = attic.number_of_files != 0;
  for(size_t i=0; i<attic.number_of_files; i++) {
    j["static_files"][i] = attic.files[i];
  }

  // Could also show other web server status/config options
  // such as: CORS, all available routes, etc.

  // or some build flags around the assets, such as an asset bundle version or so.

  serializeJson(j, res);
}

void set_cors(awot::Request& req, awot::Response& res) {
  res.set("Server", SERVER_VERSION);
  res.set("Content-Type", "application/json");
  // Disable CORS for the time being
  // Note the maximum number of headers of the lib (can be avoided by directly printing)
  res.set("Access-Control-Allow-Origin", "*");
  res.set("Access-Control-Allow-Credentials", "true");
  res.set("Access-Control-Allow-Methods", "POST, GET, OPTIONS");
  res.set("Access-Control-Allow-Headers", "Origin, Cookie, Set-Cookie, Content-Type, Server");  
}

void api_preflight(awot::Request& req, awot::Response& res) {
  res.status(200);
  set_cors(req, res);
}


void api(awot::Request& req, awot::Response& res) {
  set_cors(req, res);

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
  
  // This is more for testing, not really a good endpoint.
  webapp.get("/webserver-status", &about_static);

  // register static file handler as last one, this will also fall back to 404.
  webapp.get(serve_static);
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
