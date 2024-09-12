// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#ifdef ARDUINO

#include <Ethernet.h>
#include <QNEthernet.h>
#include <string>

#include "nvmconfig/persistent.h"
#include "utils/durations.h"
#include "utils/mac.h"
#include "utils/singleton.h"

#include "net/auth.h"

namespace net {

// import some practical classes
using qindesign::network::Ethernet;
using qindesign::network::EthernetClient;
using qindesign::network::EthernetServer;
using qindesign::network::MDNS;

using utils::MacAddress;

/// This status contains only "static" QNEthernet information, unrelated to UserDefinedEthernet instances
void status(JsonObject &msg_out);

/**
 * Persistent user-defined ethernet settings which is mostly relevant only during startup
 * as the information is subsequently hold by the QNEthernet library objects.
 * .
 *
 * \ingroup Singletons
 **/
class StartupConfig : public nvmconfig::PersistentSettings, public utils::HeapSingleton<StartupConfig> {
public:
  void reset_defaults();

  std::string name() const { return "net"; }

  /// Initializes with default values which are always valid and serve as a fallback
  /// if invalid options are given. Default values are defined at build time in the code.
  StartupConfig() { reset_defaults(); }

  bool enable_ethernet,  ///< Turn on/off networking completely
      enable_dhcp,       ///< DHCP client vs static IP configuration
      enable_jsonl,      ///< Enable the JSONL TCP/IP server
      enable_webserver,  ///< Enable embedded webserver for REST access
      enable_websockets, ///< Enable websocket server ontop of webserver
      enable_mdns;       ///< Enable mDNS/zeroconf multicast service discovery

  int jsonl_port,     ///< TCP port for jsonl server
      webserver_port; ///< TCP port for webserver

  MacAddress mac;       ///< Custom MAC address. Defaults to original permanent system Mac.
  std::string hostname; ///< used only for DHCP client. Maximum 250 characters.

  IPAddress static_ipaddr, ///< own ip address, used only when use_dhcp=false
      static_netmask,      ///< netmask; used only when use_dhcp=false
      static_gw,           ///< gateway address; used only when use_dhcp=false
      static_dns;          ///< DNS server address; used only when use_dhcp=false

  // from here on, we have runtime connection settings

  uint32_t connection_timeout_ms; ///< (Runtime-changable) Time after idling connections time out.
  uint8_t max_connections;        ///< (Runtime-changable) Maximum number of parallel connections accepted

  int begin_ip();    ///< Calls net::Ethernet.begin, sets IP address
  void begin_mdns(); ///< Calls net::MDNS.begin
  // void begin();

  void fromJson(JsonObjectConst src, nvmconfig::Context c = nvmconfig::Context::Flash) override;
  void toJson(JsonObject target, nvmconfig::Context c = nvmconfig::Context::Flash) const override;
};

JSON_CONVERT_SUGAR(StartupConfig);

/**
 * Digital networking (TCP/IP) related information which is hold in RAM and
 * can be changed at runtime in principle.
 **/
/*  class RuntimeConfig : nvmconfig::PersistentSettings {
    void reset_defaults();

    static RuntimeConfig& get() {
        static RuntimeConfig instance;
        return instance;
    }
  };
*/

} // namespace net

#endif // ARDUINO
