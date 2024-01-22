// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Ethernet.h>
#include <QNEthernet.h>
#include <ArduinoJson.h>
#include <cstring>

namespace net = qindesign::network;

namespace user {
namespace ethernet {

std::string system_mac_as_string(); //< Own mac address in Canonical Format AA-BB-CC-DD-EE-FF

void status(JsonObject &msg_out);

/**
 *  Persistent user-defined ethernet settings.
 **/
class UserDefinedEthernet {
public:

  int server_port;    ///< TCP Ethernet Server port
  bool use_dhcp;      ///< DHCP client vs static IP configuration

  String hostname;    ///< used only for DHCP client. Maximum 250 characters.

  IPAddress
      static_ipaddr,   ///< used only when use_dhcp=false
      static_netmask,  ///< used only when use_dhcp=false
      static_gw;       ///< used only when use_dhcp=false

  // TODO: Also store static_dns server name for resolving.

  void reset_defaults();

  void read_from_json(JsonObjectConst serialized_conf);
  void write_to_json(JsonObject target);

  void begin(net::EthernetServer *server);
};

} // namespace ethernet
} // namespace user
