// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Ethernet.h>
#include <QNEthernet.h>
#include <ArduinoJson.h>
#include <string>

namespace net = qindesign::network;

namespace user {
namespace ethernet {

/// Represents Ethernet Mac address
struct MacAddress {
  uint8_t mac[6] = {0};
  void reset(); ///< set to system default (stored in teensy HW_OCOTP_MAC1 and ...MAC0)
  operator std::string() const;
  bool fromString(const char* str);
  uint8_t &operator[](size_t i) { return mac[i]; }
  const uint8_t &operator[](size_t i) const { return mac[i]; }
  /// Handy access to system default/permanent mac address (one time programmed)
  static MacAddress otp() { MacAddress mac; mac.reset(); return mac; }
};

std::string toString(const MacAddress& mac, char sep='-'); ///< Canonical Format AA-BB-CC-DD-EE-FF
std::string toString(const IPAddress& ip); ///< Canonical Format 123.45.6.7

bool valid(const MacAddress& mac); ///< All zero mac bytes considered invalid (0-0-0-0-0-0)
bool valid(const IPAddress& ip);   ///< All zero IP bytes considered invalid (0.0.0.0)

/// This status contains only "static" QNEthernet information, unrelated to UserDefinedEthernet instances
void status(JsonObject &msg_out);

/**
 * Persistent user-defined ethernet settings.
 * 
 * \note Changing these settings only takes place after reboot of the microcontroller.
 **/
class UserDefinedEthernet {
public:
  /// Initializes with default values which are always valid and serve as a fallback
  /// if invalid options are given. Default values are defined at build time in the code.
  UserDefinedEthernet();

  uint16_t server_port;///< TCP Ethernet Server port
  bool use_dhcp;       ///< DHCP client vs static IP configuration

  MacAddress mac;      ///< Custom MAC address. Defaults to original permanent system Mac.

  std::string hostname; ///< used only for DHCP client. Maximum 250 characters.

  IPAddress
      static_ipaddr,   ///< own ip address, used only when use_dhcp=false
      static_netmask,  ///< netmask; used only when use_dhcp=false
      static_gw,       ///< gateway address; used only when use_dhcp=false
      static_dns;      ///< DNS server address; used only when use_dhcp=false

  /// Resets this instance to the default values.
  void reset_defaults();

  void begin(net::EthernetServer *server);
};

void convertFromJson(JsonVariantConst serialized_conf, UserDefinedEthernet& ude);
void convertToJson(const UserDefinedEthernet& ude, JsonVariant serialized);

void convertFromJson(JsonVariantConst serialized_mac, MacAddress& mac);
void convertToJson(const MacAddress& mac, JsonVariant serialized_mac);

} // namespace ethernet
} // namespace user

// must be defined in global namespace, where Arduinos IPAddress lives
void convertFromJson(JsonVariantConst ipjson, IPAddress& ip);
void convertToJson(const IPAddress& ip, JsonVariant ipjson);
