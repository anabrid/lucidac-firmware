// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// This file is part of the model-1 hybrid-controller firmware.
//
// ANABRID_BEGIN_LICENSE:GPL
// Commercial License Usage
// Licensees holding valid commercial anabrid licenses may use this file in
// accordance with the commercial license agreement provided with the
// Software or, alternatively, in accordance with the terms contained in
// a written agreement between you and Anabrid GmbH. For licensing terms
// and conditions see https://www.anabrid.com/licensing. For further
// information use the contact form at https://www.anabrid.com/contact.
//
// GNU General Public License Usage
// Alternatively, this file may be used under the terms of the GNU
// General Public License version 3 as published by the Free Software
// Foundation and appearing in the file LICENSE.GPL3 included in the
// packaging of this file. Please review the following information to
// ensure the GNU General Public License version 3 requirements
// will be met: https://www.gnu.org/licenses/gpl-3.0.html.
// For Germany, additional rules exist. Please consult /LICENSE.DE
// for further agreements.
// ANABRID_END_LICENSE

#pragma once

#include <Ethernet.h>
#include <QNEthernet.h>
#include <ArduinoJson.h>
#include <cstring>

#include "message_handlers.h"

namespace net = qindesign::network;

namespace ethernet {

std::string system_mac_as_string(); //< Own mac address in Canonical Format AA-BB-CC-DD-EE-FF

void status(JsonObject &msg_out);

class UserDefinedEthernet {
public:

  int server_port;    ///< TCP Ethernet Server port
  bool use_dhcp;      ///< DHCP client vs static IP configuration

  String hostname;    ///< used only for DHCP client. Maximum 250 characters.

  IPAddress
      static_ipaddr,   ///< used only when use_dhcp=false
      static_netmask,  ///< used only when use_dhcp=false
      static_gw;       ///< used only when use_dhcp=false

  void reset_defaults();

  void read_from_json(JsonObjectConst serialized_conf);
  void write_to_json(JsonObject target);

  void begin(net::EthernetServer *server);
};

} // namespace ethernet
