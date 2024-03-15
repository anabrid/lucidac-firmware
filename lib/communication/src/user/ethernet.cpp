#include <ArduinoJson.h>

#include "utils/logging.h"
#include "user/ethernet.h"
#include "ethernet.h"

#include "lwip_t41.h" // from QNEthernet import enet_get_mac

using namespace user::ethernet;

void user::ethernet::MacAddress::reset() {
  enet_get_mac(mac);
}

user::ethernet::MacAddress::operator std::string() const {
  return toString(*this); // not inline to avoid forward declaration
}

bool user::ethernet::MacAddress::fromString(const char *address) {
  // required format AA-BB-CC-DD-EE-FF
  if(strlen(address) != 17) return false;
  for(int i=0; i<6; i++, address+=3)
    mac[i] = std::stoul(address, nullptr, 16);
  return true;
}

std::string user::ethernet::toString(const MacAddress &mac, char sep) {
  char mac_str[20];
  sprintf(mac_str, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
    mac[0], sep, mac[1], sep, mac[2], sep, mac[3], sep, mac[4], sep, mac[5]);
  return mac_str;
}

std::string user::ethernet::toString(const IPAddress& ip) {
  char ip_str[16];
  sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return ip_str;
}


bool user::ethernet::valid(const IPAddress& ip)   { uint8_t sum=0; for(int i=0; i<4; i++) sum +=  ip[i]; return sum != 0; }
bool user::ethernet::valid(const MacAddress& mac) { uint8_t sum=0; for(int i=0; i<4; i++) sum += mac[i]; return sum != 0; }

user::ethernet::UserDefinedEthernet::UserDefinedEthernet() {
  server_port = 5732;
  connection_timeout_ms = 72*1000*1000; /// @TODO test timeout.
  max_connections = 20;

  // This build flag basically solves a "deadlock" if a no DHCP server is
  // available but the teensy is configured for dhcp.
  #ifdef ANABRID_SKIP_DHCP
  use_dhcp = false;
  LOG_ALWAYS("Skipping DHCP due to build flag ANABRID_SKIP_DHCP");
  #else
  use_dhcp = true;
  #endif

  mac.reset();

  // Note that the default hostname by QNEthernet is teensy-lwip.
  // Instead, here we choose something unique to the device.

  char mac_str[50];
  sprintf(mac_str, "lucidac-%02X-%02X-%02X", mac[3], mac[4], mac[5]);
  hostname = mac_str;

  static_ipaddr = IPAddress(192, 168, 1, 100);
  static_netmask = IPAddress(255, 255, 255, 0);
  static_gw = IPAddress(192, 168, 1, 1);
  static_dns = IPAddress(8, 8, 8, 8);
}

void user::ethernet::UserDefinedEthernet::reset_defaults() {
  *this = UserDefinedEthernet();
}

#define GET(key)         if(serialized_conf.containsKey(#key)) ude.key = serialized_conf[#key];
#define GET_AS(key,type) if(serialized_conf.containsKey(#key)) ude.key = serialized_conf[#key].as<type>();

void user::ethernet::convertFromJson(JsonVariantConst serialized_conf, user::ethernet::UserDefinedEthernet& ude) {
  GET(server_port);
  GET(use_dhcp);
  GET(mac);
  GET_AS(hostname, const char*);
  GET_AS(static_ipaddr, IPAddress);
  GET_AS(static_netmask, IPAddress);
  GET_AS(static_gw, IPAddress);
  GET_AS(static_dns, IPAddress);

  // Maximum hostname length is an industry standard (POSIX is 255 chars limits.h)
  // and also and dictated by EEPROM size.
  if(ude.hostname.length() > 250) ude.hostname = ude.hostname.substr(0, 250);

}

void user::ethernet::convertToJson(const user::ethernet::UserDefinedEthernet &ude, JsonVariant serialized) {
  auto target = serialized.to<JsonObject>();

  target["server_port"] = ude.server_port;
  target["use_dhcp"] = ude.use_dhcp;
  target["mac"] = ude.mac;
  target["hostname"] = ude.hostname;
  target["static_ipaddr"] = ude.static_ipaddr;
  target["static_netmask"] = ude.static_netmask;
  target["static_gw"] = ude.static_gw;
  target["static_dns"] = ude.static_dns;
}

void user::ethernet::convertFromJson(JsonVariantConst macjson, MacAddress &mac) {
  mac.fromString(macjson.as<const char*>());
}

void convertFromJson(JsonVariantConst ipjson, IPAddress &ip) {
    ip.fromString(ipjson.as<const char*>());
}

void convertToJson(const IPAddress &ip, JsonVariant ipjson) {
    ipjson.set(toString(ip));
}

void user::ethernet::convertToJson(const MacAddress& mac, JsonVariant macjson) {
  macjson.set(toString(mac));
}

void user::ethernet::UserDefinedEthernet::begin(net::EthernetServer *server) {
    if(valid(mac)) net::Ethernet.setMACAddress(mac.mac); // else keep system default
    *server = net::EthernetServer(server_port);

    LOG2("MAC: ", toString(mac).c_str())

    if(use_dhcp) {
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
          auto defaults = UserDefinedEthernet();
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
          auto defaults = UserDefinedEthernet();
          static_dns = defaults.static_dns;
        }
        net::Ethernet.setDnsServerIP(static_dns);
    }

    LOG4("Listening on ", net::Ethernet.localIP(), ":", server_port);

    server->begin();
}


void user::ethernet::status(JsonObject &msg_out) {
  msg_out["interfaceStatus"] = net::Ethernet.interfaceStatus();
  msg_out["otp_mac"] = std::string(MacAddress::otp()); // one time programmable system/permanent mac

  auto ip = msg_out.createNestedObject("ip");
  ip["local"] = net::Ethernet.localIP();
  ip["broadcast"] = net::Ethernet.broadcastIP();
  ip["gateway"] = net::Ethernet.gatewayIP();

  auto dhcp = msg_out.createNestedObject("dhcp");
  dhcp["active"] = net::Ethernet.isDHCPActive();
  dhcp["enabled"] = net::Ethernet.isDHCPEnabled();

  auto link = msg_out.createNestedObject("link");
  link["state"] = net::Ethernet.linkState();
  link["speed"] = net::Ethernet.linkSpeed(); // in Mpbs
  link["isCrossover"] = net::Ethernet.linkIsCrossover();
  link["isFullDuplex"] = net::Ethernet.linkIsFullDuplex();

  // msg_out["tcp_port"] = user::ethernet::Ethernet.server_port;
  //// We could also dump the settings, but instead of the "configuration state"
  //// this handler shall report the actual state.
  // auto settings = msg_out.createNestedObject("settings");
  // eth.write_to_json(settings);
}