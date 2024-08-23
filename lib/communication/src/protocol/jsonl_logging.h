// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h> // Output, millis()
#ifdef ARDUINO

#include <list>
#include <string>

#include "utils/print-multiplexer.h"
#include "utils/streaming_json.h"

namespace msg {

/** Poor man's ring buffer based on a STL list */
template <typename T> class ring_buffer {
  std::list<T> _data;

public:
  /// Read-only access to the underlying data structure
  const std::list<T> &data() const { return _data; }

  size_t max_size; ///< Maximum number of elements to hold

  ring_buffer(int max_size) : max_size(max_size) {}

  void push_back(T item) {
    if (_data.size() > max_size)
      _data.pop_front();
    _data.push_back(item);
  }

  void clear() { _data.clear(); }
};

// Decorates a stream of characters into JSONL-ready LogLines
struct StreamLogger : public Print {
  uint16_t line_count = 0;
  bool new_line = true;
  ::utils::StreamingJson target;

  StreamLogger(Print &_target) : target(_target) {}

  virtual size_t write(uint8_t b) override;
  // size_t write(const uint8_t *buffer, size_t size) override
};

/**
 * This provides a simple log facility for error reporting. By default, this
 * will be set up at the main() to log at least to the Serial console and to
 * the startup log buffer. However, also other targets such as Ethernet/TCP/IP
 * clients can be hooked.
 **/
struct Log : public Print {
  msg::StreamLogger formatter;
  utils::PrintMultiplexer sinks;

  Log() : formatter(sinks) {}

  virtual size_t write(uint8_t b) override { return formatter.write(b); }

  // for debugging...
  // Log() : msg::StreamLogger(Serial) {}

  static Log &get() {
    static Log instance;
    return instance;
  }
};

void activate_serial_log();

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
  constexpr static size_t max_buf_size = 30;
  msg::ring_buffer<std::string> buf{max_buf_size};
  std::string linebuf = "";

  StartupLog() { linebuf.reserve(80); }

  bool is_active() { return buf.max_size != 0; }

  virtual size_t write(uint8_t b) override {
    if (b == '\r')
      return 1; // Ignore CR in CR NL
    if (b == '\n') {
      buf.push_back(linebuf);
      linebuf.clear();
      return 1;
    }
    linebuf += b;
    return 1;
  }

  void disable() {
    buf.max_size = 0;
    buf.clear();
    linebuf.clear();
  }

  // Logs are big so don't use regular ArduinoJSON
  void stream_to_json(utils::StreamingJson &s) {
    s.begin_dict();
    s.kv("is_active", is_active());
    s.kv("max_size", buf.max_size);
    s.key("entries");
    s.begin_list();
    for (auto const &line : buf.data()) {
      s.json(line.c_str());
    }
    s.end_list();
    s.end_dict();
  }

  static StartupLog &get() {
    static StartupLog instance;
    return instance;
  }
};

} // namespace msg

#endif // ARDUINO
