#pragma once

#include <Arduino.h>
#ifdef ARDUINO

#include "utils/json.h"
#include <IPAddress.h>

#include <cstdint>
#include <list>
#include <string>

namespace utils {

/**
 * Represents Ethernet Mac address
 * Converts from/to canonical representation AA-BB-CC-DD-EE-FF
 **/
struct MacAddress {
  uint8_t mac[6] = {0};
  void reset(); ///< set to system default (stored in teensy HW_OCOTP_MAC1 and ...MAC0)
  operator std::string() const;
  bool fromString(const char *str);

  uint8_t &operator[](size_t i) { return mac[i]; }

  const uint8_t &operator[](size_t i) const { return mac[i]; }

  /// Handy access to system default/permanent mac address (one time programmed)
  static MacAddress otp() {
    MacAddress mac;
    mac.reset();
    return mac;
  }
};

void convertFromJson(JsonVariantConst serialized_mac, MacAddress &mac);
void convertToJson(const MacAddress &mac, JsonVariant serialized_mac);

std::string toString(const MacAddress &mac, char sep = '-'); ///< Canonical Format AA-BB-CC-DD-EE-FF
std::string toString(const IPAddress &ip);                   ///< Canonical Format 123.45.6.7

bool valid(const MacAddress &mac); ///< All zero mac bytes considered invalid (0-0-0-0-0-0)
bool valid(const IPAddress &ip);   ///< All zero IP bytes considered invalid (0.0.0.0)

/** IPv4 Adress with net mask */
struct IPAddressMask {
  IPAddress address;
  IPAddress mask;
  // TODO: fromString, toString, contains(IPAddress)
};

/** Represents a list of IP Addresses */
struct IPMaskList {
  std::list<IPAddressMask> list;
  boolean contains(IPAddress); // true if any contains
};

// TODO implement!
// Probably want to serialize as *compact* if it is eeprom facing not user facing
inline void convertFromJson(JsonVariantConst serialized_iplist, IPMaskList &iplist) {}

inline void convertToJson(const IPMaskList &iplist, JsonVariant serialized_iplist) {}

} // namespace utils

// must be defined in global namespace, where Arduinos IPAddress lives
void convertFromJson(JsonVariantConst ipjson, IPAddress &ip);
void convertToJson(const IPAddress &ip, JsonVariant ipjson);

#endif // ARDUINO
