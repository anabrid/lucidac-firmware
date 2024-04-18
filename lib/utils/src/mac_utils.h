// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
//
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <string>

#ifdef ARDUINO

#include <cstring>
#include <QNEthernet.h>

std::string get_system_mac() {
  uint8_t mac[6];
  qindesign::network::Ethernet.macAddress(mac);
  char mac_str[20];
  sprintf(mac_str, "%02X-%02X-%02X-%02X-%02X-%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  return mac_str;
}

#else

std::string get_system_mac() { return "12-34-56-78-9A-BC"; }

#endif