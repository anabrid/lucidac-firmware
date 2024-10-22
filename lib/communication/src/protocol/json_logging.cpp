// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#include "protocol/jsonl_logging.h"

#ifdef ARDUINO

FLASHMEM size_t msg::StreamLogger::write(uint8_t b) {
  // size_t write(const uint8_t *buffer, size_t size) override {
  if (new_line) {
    target.begin_dict();
    target.kv("type", "log");
    target.kv("count", line_count++);
    target.kv("time", millis());
    target.key("msg");
    target.begin_str();
    new_line = false;
  }
  if (b == '\r')
    return 1; // ignore CR in CR NL line endings.
  if (b == '\n') {
    target.end_str();
    target.end_dict();
    target.output.println("");
    target.output.flush();
    new_line = true;
    return 1;
  } else {
    return target.output.write(b);
  }
}

FLASHMEM void msg::StartupLog::stream_to_json(utils::StreamingJson &s) {
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

void msg::activate_serial_log() { msg::Log::get().sinks.add_Serial(); }

#endif // ARDUINO
