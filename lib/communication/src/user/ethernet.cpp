#include <ArduinoJson.h>

#include "utils/logging.h"
#include "user/ethernet.h"

std::string user::ethernet::system_mac_as_string() {
  uint8_t mac[6];
  qindesign::network::Ethernet.macAddress(mac);
  char mac_str[20];
  sprintf(mac_str, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return mac_str;
}

void dump_ip(const IPAddress& ip, JsonArray target) {
    for(int i=0; i<4; i++) target[i] = ip[i];
}

void user::ethernet::UserDefinedEthernet::reset_defaults() {
  server_port = 5732;
  use_dhcp = true;

  // Note that the default hostname by QNEthernet is teensy-lwip.
  // Instead, here we choose something unique to the device.

  byte mac[6];
  char mac_str[50];
  net::Ethernet.macAddress(mac);
  sprintf(mac_str, "lucidac-%02X-%02X-%02X", mac[3], mac[4], mac[5]);
  hostname = mac_str;

  static_ipaddr = IPAddress(192, 168, 1, 100);
  static_netmask = IPAddress(255, 255, 255, 0);
  static_gw = IPAddress(192, 168, 1, 1);
}

void user::ethernet::UserDefinedEthernet::read_from_json(JsonObjectConst serialized_conf) {
    if(serialized_conf.containsKey("server_port"))
        server_port = serialized_conf["server_port"];
    if(serialized_conf.containsKey("use_dhcp"))
        use_dhcp = serialized_conf["use_dhcp"];

    if(serialized_conf.containsKey("hostname")) {
        hostname = serialized_conf["hostname"].as<std::string>().c_str();
    }

    for(int i=0; i<4; i++) {
        if(serialized_conf.containsKey("static_ipaddr"))
            static_ipaddr[i] = serialized_conf["static_ipaddr"][i];
        if(serialized_conf.containsKey("static_netmask"))
            static_netmask[i] = serialized_conf["static_netmask"][i];
        if(serialized_conf.containsKey("static_gw"))
            static_gw[i] = serialized_conf["static_gw"][i];
    }

    // TODO: Communicate that changing these settings only takes place after reboot.
}

void user::ethernet::UserDefinedEthernet::write_to_json(JsonObject target) {
    target["server_port"] = server_port;
    target["use_dhcp"] = use_dhcp;
    // TODO: Ensure that hostname is not longer then 250 chars, which is both an
    //       industry standard (POSIX is 255 chars limits.h) and dictated by EEPROM size.
    target["hostname"] = hostname;
    for(int i=0; i<4; i++) {
        target["static_ipaddr"][i] = static_ipaddr[i];
        target["static_netmask"][i] = static_netmask[i];
        target["static_gw"][i] = static_gw[i];
    }
}

void user::ethernet::UserDefinedEthernet::begin(net::EthernetServer *server) {
    *server = net::EthernetServer(server_port);

    LOG2("MAC: ", user::ethernet::system_mac_as_string().c_str())

    // This build flag basically solves a "deadlock" if a no DHCP server is
    // available but the teensy is configured for dhcp.
    #ifdef ANABRID_SKIP_DHCP
    use_dhcp = false;
    LOG_ALWAYS("Skipping DHCP due to build flag ANABRID_SKIP_DHCP");
    #endif

    if(use_dhcp) {
        LOG2("DHCP with Hostname: ", hostname);
        net::Ethernet.setHostname(hostname.c_str());
        if (!net::Ethernet.begin()) {
        LOG_ERROR("Error starting ethernet DHCP client.");
        _ERROR_OUT_
        }
        LOG(ANABRID_DEBUG_INIT, "Waiting for IP address on ethernet...");
        if (!net::Ethernet.waitForLocalIP(10000)) {
        LOG_ERROR("Error getting IP address.");
        _ERROR_OUT_
        }
    } else {
        if (!net::Ethernet.begin(static_ipaddr, static_netmask, static_gw)) {
        LOG_ERROR("Error starting ethernet with static IP address.");
        _ERROR_OUT_
        }
    }

    LOG4("Listening on ", net::Ethernet.localIP(), ":", server_port);

    (*server).begin();
}

void user::ethernet::status(JsonObject &msg_out) {
    msg_out["interfaceStatus"] = net::Ethernet.interfaceStatus();
    msg_out["mac"] = user::ethernet::system_mac_as_string();
    //msg_out["tcp_port"] = net::Ethernet.server_port;

    auto ip = msg_out.createNestedObject("ip");
    dump_ip(net::Ethernet.localIP(), ip.createNestedArray("local"));
    dump_ip(net::Ethernet.broadcastIP(), ip.createNestedArray("broadcast"));
    dump_ip(net::Ethernet.gatewayIP(), ip.createNestedArray("gateway"));
    
    auto dhcp = msg_out.createNestedObject("dhcp");
    dhcp["active"] = net::Ethernet.isDHCPActive();
    dhcp["enabled"] = net::Ethernet.isDHCPEnabled();

    auto link = msg_out.createNestedObject("link");
    link["state"] = net::Ethernet.linkState();
    link["speed"] = net::Ethernet.linkSpeed(); // in Mpbs
    link["isCrossover"] = net::Ethernet.linkIsCrossover();
    link["isFullDuplex"] = net::Ethernet.linkIsFullDuplex();

    //// We could also dump the settings, but instead of the "configuration state"
    //// this handler shall report the actual state.
    // auto settings = msg_out.createNestedObject("settings");
    // eth.write_to_json(settings);
}