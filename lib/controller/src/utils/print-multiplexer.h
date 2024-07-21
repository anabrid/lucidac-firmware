// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>
#include <list>
#include <vector>
#include <algorithm>

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
   * See also https://github.com/bblanchon/ArduinoStreamUtils for something
   * more sane.
   */
  class PrintMultiplexer : public Print {
    std::list<Print*> targets;

  public:
    void add(Print* target) { targets.push_back(target); }
    void remove(Print* target) { targets.remove(target); }
    
    // Printables which go to all clients
    virtual size_t write(uint8_t b) override {
        std::vector<size_t> ret;
        for(auto& target : targets) if(target) ret.push_back(target->write(b));
        if(!std::all_of(ret.cbegin(), ret.cend(), [](size_t i) { return i == 1; })) {
           // TODO: For circular dependency reasons, cannot use the Logging facilities because
           //       this is part of it.
           //LOG_ALWAYS("PrintMultiplexer: write(byte) failed for at least one target.");
        }
        return ret.size()>0 ? ret[0] : /*devnull*/ 1;
    }
    
    size_t write(const uint8_t *buffer, size_t size) override {
        std::vector<size_t> ret;
        for(auto& target : targets) if(target) ret.push_back(target->write(buffer, size));
        if(!std::all_of(ret.cbegin(), ret.cend(), [size](size_t i) { return i == size; })) {
           // TODO: For circular dependency reasons, cannot use the Logging facilities because
           //       this is part of it.
            //LOG_ALWAYS("PrintMultiplexer: write(buffer,size) failed for at least one target.");

            //Serial.printf("size=%d\n", size);
            //for(auto & r : ret) { Serial.printf("ret = %d\n", r); }
        }
        return ret.size()>0 ? ret[0] : size;
    }
    virtual void flush() override {
        for(auto& target : targets) if(target)  target->flush();
    }
  };


} // utils