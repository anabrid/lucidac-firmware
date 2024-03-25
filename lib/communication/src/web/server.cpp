#include "user/settings.h"
#include "web/server.h"
#include "web/assets.h"
#include "web/websockets.h"
#include "utils/logging.h"
#include "protocol/protocol.h"
#include "build/distributor.h"

#define ENABLE_AWOT_NAMESPACE
#include "web/aWOT.h"

// more readable c string algebra
bool contains(const char* haystack, const char* needle) { return strstr(haystack, needle) != NULL; }
bool equals(const char* a, const char* b) { return strcmp(a, b) == 0; }

using namespace web;

#define SERVER_VERSION  "LucidacWebServer/" FIRMWARE_VERSION

// TODO: Redeem this pseudo singleton architecture

struct HTTPContext {
  net::EthernetClient* client;
  LucidacWebServer* server;
  bool convert_to_websocket;
};

awot::Application webapp;

/**
 * aWOT preallocates everything, therefore request headers are not dynamically parsed
 * but instead only seved when previously storage was allocated. Therefore this function
 * serves as a global registry for interesting client-side headers.
 **/
void allocate_interesting_headers(awot::Application& app) {
  constexpr int max_allocated_headers = 10;
  constexpr int maxval = 80; // maximum string length

  int i=0;

  // This is the storage for the headers. Don't confuse it with the linked
  // list managed internally by awot::Appplication which only stores string
  // pointers back to this storage.
  static char header[max_allocated_headers][maxval];

  app.header("Host", header[i++], maxval);
  app.header("Origin", header[i++], maxval);
  app.header("Connection", header[i++], maxval);
  app.header("Upgrade", header[i++], maxval);
  app.header("Sec-WebSocket-Version", header[i++], maxval);
  app.header("Sec-Websocket-Key", header[i++], maxval);

  // programmer, please ensure at this line i < max_allocated_headers.
}

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
  // // Note on the note: This use case is possible with the aWOT API, cf.
  // // https://awot.net/en/3x/api.html#res-beginHeaders
  for(
    uint32_t written = 0;
    written != file.size;
  ) {
    // The folling line requires anabrid/aWOT@3.5.1 as upstream aWOT 3.5.0
    // has a severe bug. Fix is sketched in 
    // https://github.com/lasselukkari/aWOT/compare/master...lfarrand:aWOT:master
    // and adopted by us.
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
  if(res.bytesSent() || res.ended()) return;

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
    j["static_files"][i] = attic.files[i].filename;
  }

  // Note that this dump is already truncated by JSON Document size.
  // Question: Do we even need this functionality?

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


#define ERR(msg) { \
  if(!has_errors){has_errors=true; res.status(400); res.set("Content-Type", "text/plain"); \
    res.println("This URL is only for Websocket Upgrade."); }  \
  res.println("Error: " msg); }

void websocket_upgrade(awot::Request& req, awot::Response& res) {
  res.set("Server", SERVER_VERSION);
  auto ctx = (HTTPContext*) req.context;

  // TODO: Check req.get("Origin") if it is allowed to connect.
  // Similar as in the CORS, have to keep a user setting of allowed origins.

  bool has_errors = false;
  if(!req.get("Connection")) ERR("Missing 'Connection' header");
  if(!contains(req.get("Connection"), "Upgrade")) ERR("Connection header contains not 'Upgrade'");
  if(!req.get("Upgrade")) ERR("Missing 'Upgrade' header");
  if(!equals(req.get("Upgrade"), "websocket")) ERR("Upgrade header is not 'websocket'");
  if(!req.get("Sec-WebSocket-Version")) ERR("Missing Sec-Websocket-Version");
  if(!equals(req.get("Sec-WebSocket-Version"), "13")) ERR("Only Sec-Websocket-Version=13 supported");
  if(!req.get("Sec-WebSocket-Key")) ERR("Missing Sec-Websocket-Key");
  if(has_errors) return;

  if(ctx->server->clients.size() == user::UserSettings.ethernet.max_connections) {
      LOG3("Cannot accept new websocket connection because maximum number of connections (",
        user::UserSettings.ethernet.max_connections, ") already reached.");
      res.status(500);
      res.set("Content-Type", "text/plain");
      res.println("Maximum number of concurrent websocket connections reached");
      return;
  }

  auto serverAccept = websocketsHandshakeEncodeKey(req.get("Sec-WebSocket-Key"));

  res.set("Connection", "Upgrade");
  res.set("Upgrade", "websocket");
  res.set("Sec-WebSocket-Version", "13");
  res.set("Sec-Websocket-Accept", serverAccept.c_str());
  res.status(101); // Switching Protocols
  res.flush();
  res.end(); // avoid the catchall handler to get active

  ctx->convert_to_websocket = true;
  // Control to WebsocketsClient is passed at further LucidacWebServer::loop iterations.
}

void web::LucidacWebServer::begin() {
  ethserver.begin(80);

  allocate_interesting_headers(webapp);

  webapp.get("/", &index);

  webapp.get("/api", &api);
  webapp.post("/api", &api);
  webapp.options("/api", &api_preflight);

  webapp.get("/websocket", &websocket_upgrade);
  webapp.options("/websocket", &api_preflight);
  
  // This is more for testing, not really a good endpoint.
  webapp.get("/webserver-status", &about_static);

  // register static file handler as last one, this will also fall back to 404.
  webapp.get(serve_static);
}

// ws initialization looks a bit bonkers, so let me explain it:
// The communciation/src/websockets package is alien and not integrated into the codebase.
// It comes with its own Socket-handling logic. There, there is a TcpClient which holds a pointer
// to our QNEthernet client. However, the TcpClient itself is a shared pointer for whatever reason.
// So for letting websocket clients lookup client context informations, we have this link-loop
web::LucidacWebsocketsClient::LucidacWebsocketsClient(const net::EthernetClient &other) :
  socket(other),
  ws(std::shared_ptr<websockets::network::TcpClient>(new websockets::network::TcpClient(this))) {
  user_context.set_remote_identifier( user::auth::RemoteIdentifier{ socket.remoteIP() } );
}

/*
  Note that this data handling is *REALLY* not performant, as it requires a websocket message to be completely
  read into a string, then passed over to the JSON parser which again parses it into its buffer (note that
  ArduinoJSON is *actually* streamlined for reading things from/to buffers) and then all the way back.
  However: It works, at least.

  Potential caveat: Users sending really large messages. Check if there is some safety net for that in
      our websockets library.
*/
void onWebsocketMessageCallback(websockets::WebsocketsClient& wsclient, websockets::WebsocketsMessage msg) {
  LOGMEV("ws Role=%d, data=%s", msg.role(), msg.data().c_str());
  if(!msg.isComplete()) { LOG_ALWAYS("webSockets: Ignoring incomplete message"); return; }
  if(!msg.isText()) { LOG_ALWAYS("webSockets: Ignoring non-text message"); return; }
 
  std::string envelope_out_str;
  msg::JsonLinesProtocol::get().process_string_input(msg.data(), envelope_out_str, wsclient.client()->context->user_context);
  wsclient.send(envelope_out_str);
}

void web::LucidacWebServer::loop() {
  net::EthernetClient client_socket = ethserver.accept();

  // Warning: This probably allows also to deadlock the device by opening a connection
  //          but not sending anything.

  if(client_socket) {
    // incoming new HTTP client.
    LOG4("Web Client request from ", client_socket.remoteIP(), ":", client_socket.remotePort());

    HTTPContext ctx;
    ctx.server = this;
    ctx.client = &client_socket;
    ctx.convert_to_websocket = false;
    
    webapp.process(&client_socket, &ctx);

    if(ctx.convert_to_websocket) {
      LOG_ALWAYS("Accepting Websocket connection.");

      // we use emplace + a constructor because we have to be super careful about having pointers
      // referencing to the correct socket.
      LucidacWebsocketsClient& client = clients.emplace_back(client_socket);

      LOG_ALWAYS("Pushed to clients list.");

      // Don't use masking from server to client (according to RFC)
      client.ws.setUseMasking(false);

      // register callback for receiving messages.
      client.ws.onMessage(onWebsocketMessageCallback);

      // Send a hello world or so
      client.ws.send("{'hello':'client'}\n");

      // TODO: Consider adding to JsonLinesProtocol::get().broadcast,
      //       however would require some callback because message needs to be wrapped
      //       into websocket protocol.

      LOG_ALWAYS("Done accepting Websocket connection.");
    } else {
      // As we currently do not support keep-alive, we need to close the 
      // connection, otherwise browser wait for more data.
      client_socket.close();
    }
  }

  // using iterator instead range-based loop because EthernetClient lacks == operator
  // so we use list::erase instead of list::remove.
  for(auto client = clients.begin(); client != clients.end(); client++) {
    const auto client_idx = std::distance(clients.begin(), client);
    if(client->socket.connected()) {
      if(client->ws.available()) {
        //msg::JsonLinesProtocol::get().process_tcp_input(client->socket, client->user_context);
        //webapp.process(&client->socket);
        client->ws.poll();
        // note that actual message handling is done via the onWebsocketMessageCallback.
        client->last_contact.reset();
      } else if(!client->socket.connected()) {
        client->socket.stop();
      } else if(client->last_contact.expired(user::UserSettings.ethernet.connection_timeout_ms)) {
        LOG5("Web client ", client_idx, ", timed out after ", user::UserSettings.ethernet.connection_timeout_ms, " ms of idling");
        client->ws.close(websockets::CloseReason_GoingAway);
        // consider...
        // client->socket.stop();
      }
    } else {
      LOG5("Websocket Client ", client_idx, ", was ", client->socket.remoteIP(), ", disconnected");
      client->socket.close();
      //JsonLinesProtocol::get().broadcast.remove(&client->socket);
      clients.erase(client);
      return; // iterator invalidated, better start loop() freshly.
    }
  }
}

