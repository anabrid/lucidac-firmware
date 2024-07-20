#include <ArduinoJson.h>

#include "utils/logging.h"
#include "net/ethernet.h"
#include "net/settings.h"

#include "protocol/jsonl_server.h"
#include "web/server.h"

using namespace utils;

void net::StartupConfig::reset_defaults() {
  #ifdef ANABRID_SKIP_ETHERNET
  enable_ethernet = false;
  LOG_ALWAYS("Skipping Ethernet due to build flag ANABRID_ETHERNET");
  #else
  enable_ethernet = true;
  #endif

  // This build flag basically solves a "deadlock" if a no DHCP server is
  // available but the teensy is configured for dhcp.
  #ifdef ANABRID_SKIP_DHCP
  enable_dhcp = false;
  LOG_ALWAYS("Skipping DHCP due to build flag ANABRID_SKIP_DHCP");
  #else
  enable_dhcp = true;
  #endif

  mac.reset();

  // Note that the default hostname by QNEthernet is just "teensy-lwip".
  // Instead, here we choose something unique to the device.
  char mac_str[50];
  sprintf(mac_str, "lucidac-%02X-%02X-%02X", mac[3], mac[4], mac[5]);
  hostname = mac_str;

  static_ipaddr = IPAddress(192, 168, 1, 100);
  static_netmask = IPAddress(255, 255, 255, 0);
  static_gw = IPAddress(192, 168, 1, 1);
  static_dns = IPAddress(8, 8, 8, 8);

  enable_jsonl = true;
  enable_webserver = true;
  enable_websockets = true;
  enable_mdns = true;

  jsonl_port = 5732;
  webserver_port = 80;

  connection_timeout_ms = 72*1000*1000; /// @TODO test timeout.
  max_connections = 20;
}



void net::StartupConfig::fromJson(JsonObjectConst src, nvmconfig::Context c) {
  JSON_GET(src, enable_ethernet);
  JSON_GET(src, enable_dhcp);
  JSON_GET(src, enable_jsonl);
  JSON_GET(src, enable_webserver);
  JSON_GET(src, enable_websockets);
  JSON_GET(src, enable_mdns);
  JSON_GET(src, jsonl_port);
  JSON_GET(src, webserver_port);
  JSON_GET(src, mac);
  JSON_GET_AS(src, hostname, const char*);
  JSON_GET_AS(src, static_ipaddr, IPAddress);
  JSON_GET_AS(src, static_netmask, IPAddress);
  JSON_GET_AS(src, static_gw, IPAddress);
  JSON_GET_AS(src, static_dns, IPAddress);

  // Maximum hostname length is an industry standard (POSIX is 255 chars limits.h)
  // and also and dictated by EEPROM size.
  if(hostname.length() > 250) hostname = hostname.substr(0, 250);
}

void net::StartupConfig::toJson(JsonObject target, nvmconfig::Context c) const {
  JSON_SET(target, enable_ethernet);
  JSON_SET(target, enable_dhcp);
  JSON_SET(target, enable_jsonl);
  JSON_SET(target, enable_webserver);
  JSON_SET(target, enable_websockets);
  JSON_SET(target, enable_mdns);
  JSON_SET(target, jsonl_port);
  JSON_SET(target, webserver_port);
  JSON_SET(target, mac);
  JSON_SET(target, hostname);
  JSON_SET(target, static_ipaddr);
  JSON_SET(target, static_netmask);
  JSON_SET(target, static_gw);
  JSON_SET(target, static_dns);
}


void net::StartupConfig::begin_ip() {
  if(!enable_ethernet) {
    LOG_ALWAYS("Ethernet disabled by user setting");
  }

  if(valid(mac)) net::Ethernet.setMACAddress(mac.mac); // else keep system default

  LOG2("MAC: ", toString(mac).c_str())

  // TODO: Should indicate the state of the IP aquisition on the
  //       LEDs of the LUCIDAC front.

  if(enable_dhcp) {
      LOG2("DHCP with Hostname: ", hostname.c_str());
      net::Ethernet.setHostname(hostname.c_str());
      if (!net::Ethernet.begin()) {
        LOG_ERROR("Error starting ethernet DHCP client.");
        //_ERROR_OUT_
      }
      LOG(ANABRID_DEBUG_INIT, "Waiting for IP address on ethernet...");
      if (!net::Ethernet.waitForLocalIP(3 /* seconds*/)) {
        LOG_ERROR("Error getting IP address.");
        //_ERROR_OUT_
      }
  } else {
      if(!valid(static_ipaddr) || !valid(static_netmask) || !valid(static_gw)) {
        LOG_ERROR("Illegal ipaddr/netmask/gw. Recovering with defaults.");
        auto defaults = StartupConfig();
        static_ipaddr = defaults.static_ipaddr;
        static_netmask = defaults.static_netmask;
        static_gw = defaults.static_gw;
      }
      if (!net::Ethernet.begin(static_ipaddr, static_netmask, static_gw)) {
        LOG_ERROR("Error starting ethernet with static IP address.");
        //_ERROR_OUT_
      }
      if(!valid(static_dns)) {
        LOG_ERROR("Illegal dns server. Recovering with defaults.")
        auto defaults = StartupConfig();
        static_dns = defaults.static_dns;
      }
      net::Ethernet.setDnsServerIP(static_dns);
  }

  LOG4("JSONL Listening on ", net::Ethernet.localIP(), ":", jsonl_port);
}

void net::StartupConfig::begin_mdns() {
  MDNS.begin(hostname.c_str());
  if(enable_jsonl)
    MDNS.addService("_lucijsonl", "_tcp", jsonl_port);
  if(enable_webserver)
    MDNS.addService("_http", "_tcp", webserver_port);
}

void net::StartupConfig::begin() {
  net::register_settings();
  begin_ip();
}

void net::status(JsonObject &msg_out) {
  msg_out["interfaceStatus"] = net::Ethernet.interfaceStatus();

  // TODO: Move to net_get
  msg_out["otp_mac"] = std::string(MacAddress::otp()); // one time programmable system/permanent mac

  auto ip = msg_out.createNestedObject("ip");
  ip["local"] = net::Ethernet.localIP();
  ip["broadcast"] = net::Ethernet.broadcastIP();
  ip["gateway"] = net::Ethernet.gatewayIP();

  auto dhcp = msg_out.createNestedObject("dhcp");
  dhcp["active"] = net::Ethernet.isDHCPActive(); // probably move to net_get
  dhcp["enabled"] = net::Ethernet.isDHCPEnabled();

  auto link = msg_out.createNestedObject("link");
  link["state"] = net::Ethernet.linkState();
  link["speed"] = net::Ethernet.linkSpeed(); // in Mpbs
  link["isCrossover"] = net::Ethernet.linkIsCrossover();
  link["isFullDuplex"] = net::Ethernet.linkIsFullDuplex();

  // msg_out["tcp_port"] = net::ethernet::Ethernet.server_port;
  //// We could also dump the settings, but instead of the "configuration state"
  //// this handler shall report the actual state.
  // auto settings = msg_out.createNestedObject("settings");
  // eth.write_to_json(settings);
}
