// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <list>
#include <string>
#include <Arduino.h> // Output, millis()

#include "utils/print-multiplexer.h"
#include "utils/streaming_json.h"

namespace msg {

  /** Poor man's ring buffer based on a STL list */
  template<typename T>
  struct ring_buffer : std::list<T> {
    int max_size; ///< Maximum number of elements to hold
    ring_buffer(int max_size) : max_size(max_size) {}
    void push_back(T item) {
      if(size() > max_size) pop_front();
      std::list<T>::push_back(item);
    }
  };

  // Decorates a stream of characters into JSONL-ready LogLines
  struct StreamLogger : public Print {
    uint16_t line_count = 0;
    bool has_line = false;
    ::utils::StreamingJson target;

    StreamLogger(Print& _target) : target(_target) {}

    virtual size_t write(uint8_t b) override {
    //size_t write(const uint8_t *buffer, size_t size) override {
       if(!has_line) {
          target.begin_dict();
          target.kv("type", "log");
          target.kv("count", line_count++);
          target.kv("time", millis());
          target.key("msg");
          target.begin_str();
       }
       if(b == '\n') {
          target.end_str();
          target.end_dict();
          target.output.println("");
          target.output.flush();
          has_line = false;
          return 1;
       } else {
          return target.output.write(b);
       }
    }
  };

  /**
   * This class holds a finite size log (i.e. list of strings) as a ring buffer.
   * This log is supposed to be wiped as soon as the RAM is needed, i.e. at data aquisition.
   * 
   * We log full JSON messages here, which is not so memory efficient. Could do better.
   * 
   * Don't use this class directly, log to the Log class instead.
   *
   * There is only one instance to simplify access along the code.
   * Should be probably improved.
   **/
  struct StartupLog : public Print {
    msg::ring_buffer<std::string> buf{30};
    std::string linebuf = "";

    bool is_active() { return buf.max_size != 0; }

    virtual size_t write(uint8_t b) override {
      linebuf += b;
      if(b == '\n') {
        buf.push_back(linebuf);
        linebuf.clear();
      }
      return 1;
    }

    void disable() {
        buf.max_size = 0;
        buf.clear();
        linebuf.clear();
    }

    // Logs are big so don't use regular ArduinoJSON
    void stream_to_json(Print& output) {
        ::utils::StreamingJson s(output);
        s.begin_dict();
        s.kv("is_active", is_active());
        s.kv("max_size", buf.max_size);
        s.key("entries");
        s.begin_list();
        for (auto const& line : buf) {
          s.json(line.c_str());
        }
        s.end_list();
        s.end_dict();
    }

    static StartupLog& get() {
        static StartupLog instance;
        return instance;
    }
  };

  /**
   * This provides a simple log facility for error reporting. By default, this
   * will be set up at the main() to log at least to the Serial console and to
   * the startup log buffer. However, also other targets such as Ethernet/TCP/IP
   * clients can be hooked.
   **/
  struct Log : public msg::StreamLogger {
    ::utils::PrintMultiplexer sinks;

    Log() : msg::StreamLogger(sinks) {
      // The Serial console is a default target attached in the main() function.
    }

    static Log& get() {
        static Log instance;
        return instance;
    }
  };

}