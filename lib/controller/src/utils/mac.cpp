#include "utils/mac.h"
#include <QNEthernet.h>

#include "lwip_t41.h" // from QNEthernet import enet_get_mac

using namespace utils;

void utils::MacAddress::reset() {
  enet_get_mac(mac);
}

utils::MacAddress::operator std::string() const {
  return toString(*this); // not inline to avoid forward declaration
}

/** required format AA-BB-CC-DD-EE-FF */
bool utils::MacAddress::fromString(const char *address) {
  if(strlen(address) != 17) return false;
  for(int i=0; i<6; i++, address+=3)
    mac[i] = std::stoul(address, nullptr, 16);
  return true;
}

std::string utils::toString(const MacAddress &mac, char sep) {
  char mac_str[20];
  sprintf(mac_str, "%02X%c%02X%c%02X%c%02X%c%02X%c%02X",
    mac[0], sep, mac[1], sep, mac[2], sep, mac[3], sep, mac[4], sep, mac[5]);
  return mac_str;
}

std::string utils::toString(const IPAddress& ip) {
  char ip_str[16];
  sprintf(ip_str, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
  return ip_str;
}


bool utils::valid(const IPAddress& ip)   { uint8_t sum=0; for(int i=0; i<4; i++) sum +=  ip[i]; return sum != 0; }
bool utils::valid(const MacAddress& mac) { uint8_t sum=0; for(int i=0; i<4; i++) sum += mac[i]; return sum != 0; }


void utils::convertFromJson(JsonVariantConst macjson, MacAddress &mac) {
  mac.fromString(macjson.as<const char*>());
}

void convertFromJson(JsonVariantConst ipjson, IPAddress &ip) {
    ip.fromString(ipjson.as<const char*>());
}

void convertToJson(const IPAddress &ip, JsonVariant ipjson) {
    ipjson.set(toString(ip));
}

void utils::convertToJson(const MacAddress& mac, JsonVariant macjson) {
  macjson.set(toString(mac));
}