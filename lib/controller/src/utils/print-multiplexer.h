// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>

#ifdef ARDUINO

#include <algorithm>
#include <list>
#include <vector>

#include "util/PrintUtils.h" // QNEthernet

//#include "utils/logging.h"

namespace utils {

/**
 * A "multiplexer" for Print targets.
 *
 * This class tries to do something which does not properly work this way,
 * as it would require buffering. This code naively keeps "fingers crossed",
 * at least does some basic error reporting.
 *
 * TODO: Could be improved with accepting EthernetClients which have a writeFully
 *       method.
 *
 * TODO: This code is awful. Improve in any manner.
 *
 * See also https://github.com/bblanchon/ArduinoStreamUtils for something
 * more sane.
 */
class PrintMultiplexer : public Print {
  std::list<Print *> targets;
  /*
  std::vector<size_t> ret_values;

  void prepare_ret_values(size_t expected) {
    if(ret_values.size() != targets.size()) {
      ret_values.resize(targets.size());
    }
    for(size_t i=0; i < ret_values.size(); i++)
      ret_values[i] = expected;
  }
  */

public:
  void add(Print *target) { targets.push_back(target); }

  void remove(Print *target) { targets.remove(target); }

  size_t size() const { return targets.size(); }

  // Printables which go to all clients
  virtual size_t write(uint8_t b) override {
    // prepare_ret_values(1);
    bool success = true;
    for (auto &target : targets)
      if (target)
        success &= target->write(b) == 1;
    // if(!std::all_of(ret_values.cbegin(), ret_values.cend(), [](size_t i) { return i == 1; })) {
    //  TODO: For circular dependency reasons, cannot use the Logging facilities because
    //        this is part of it.
    // LOG_ALWAYS("PrintMultiplexer: write(byte) failed for at least one target.");
    //}
    return success ? 1 : 0; // ret_values.size()>0 ? ret_values[0] : /*devnull*/ 1;
  }

  size_t write(const uint8_t *buffer, size_t size) override {
    // prepare_ret_values(size);
    bool success = true;
    for (auto &target : targets)
      if (target)
        success &= qindesign::network::util::writeFully(*target, buffer, size) == size;
    // if(!std::all_of(ret_values.cbegin(), ret_values.cend(), [size](size_t i) { return i == size; })) {
    //  TODO: For circular dependency reasons, cannot use the Logging facilities because
    //        this is part of it.
    // LOG_ALWAYS("PrintMultiplexer: write(buffer,size) failed for at least one target.");

    // Serial.printf("size=%d\n", size);
    // for(auto & r : ret) { Serial.printf("ret = %d\n", r); }
    //      }
    return success ? size : 0;
    // return ret_values.size()>0 ? ret_values[0] : /*devnull*/ size;
  }

  virtual void flush() override {
    for (auto &target : targets)
      if (target)
        target->flush();
  }
};

} // namespace utils

#endif // ARDUINO
