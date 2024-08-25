// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>

#ifdef ARDUINO

#include <algorithm>
#include <list>
#include <vector>
#include <QNEthernetClient.h>

#include "util/PrintUtils.h" // QNEthernet

namespace utils {

using qindesign::network::EthernetClient;

/**
 * A "multiplexer" for Print targets.
 *
 * This class tries to do something which does not properly work this way,
 * as it would require buffering. This code naively keeps "fingers crossed"
 * because even our error reporting (logging) eventually calls it, so there is
 * no suitable error reporting anyway.
 * 
 * Methods in this class lie about their return value. This will soak up
 * data silently. Or result in multiple prints, depending on your preference.
 *
 * TODO: This code is awful. Improve in any manner.
 *
 * See also https://github.com/bblanchon/ArduinoStreamUtils for something
 * more sane.
 */
class PrintMultiplexer : public Print {
  std::list<Print *> print_targets;
  std::list<EthernetClient*> eth_targets;

public:
  bool optimistic = false; ///< whether you prefer to send too much or too few

  void add(Print *target) { print_targets.push_back(target); }
  void add(EthernetClient *target) { eth_targets.push_back(target); }

  void remove(Print *target) { print_targets.remove(target); }
  void remove(EthernetClient *target) { eth_targets.push_back(target); }

  size_t size() const { return print_targets.size() + eth_targets.size(); }

  /// Printables which go to all clients
  virtual size_t write(uint8_t b) override {
    bool success = true;
    for (auto &target : print_targets) if (target) success &= target->write(b) == 1;
    for (auto &target :   eth_targets) if (target) success &= target->write(b) == 1;
    return success ? 1 : (optimistic ? 1 : 0);
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    bool success = true;
    for (auto &target : print_targets) if (target) success &= qindesign::network::util::writeFully(*target, buffer, size) == size;
    for (auto &target :   eth_targets) if (target) success &= target->writeFully(buffer, size) == size;
    // so do all this extra work of print_targets + eth_targets just to exploit that
    // EthernetClient::write_fully internally does a "still connected" check and if
    // not, fails. This way we avoid an endless loop if a client disconnects during printout.
    // Typically, in contrast, print_targets are either Serial or String-Buffers.
    return success ? size : (optimistic ? size : 0);
  }

  virtual void flush() override {
    for (auto &target : print_targets) if (target) target->flush();
    for (auto &target :   eth_targets) if (target) target->flush();
  }
};

} // namespace utils

#endif // ARDUINO
