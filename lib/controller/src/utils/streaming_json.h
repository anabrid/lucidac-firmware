// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <Arduino.h>

namespace utils {


  /**
   * The Streaming JSON API provides a way of constructing (writing) JSON messages
   * without RAM overhead. In contrast to ArduinoJSON, there is no document which needs
   * to be represented in memory and which is setup before serialization. Instead,
   * serialization and printing happens here at method call time. Ideally, this uses the
   * Arduino Print infrastructure and thus avoids any string representations where possible.
   * 
   * The class stores only a minimal state of parsing. JSON does not allow trailing commas
   * so this is avoided with a simple state boolean. The lib is not aware of whether it
   * constructs a map/dictionary or a list/array. It is up to the user to make sure the dict
   * follows the key-value-key-value semantics.
   * 
   * Correct usage example:
   * 
   *   StreamingJson s(some_client_or_so);
   *   s.begin_dict();
   *   s.key("foo");
   *   s.val("bar");
   *   s.key("biz");
   *   s.begin_list();
   *   s.val(123);
   *   s.val(true);
   *   s.val(3.14);
   *   s.null();
   *   s.end_list();
   *   s.end_dict();
   * 
   * should create the following JSON document:
   * 
   *   {"foo":"bar","biz":[123, true, 3.14, null]}
   * 
   **/
  class StreamingJson {
    bool _needs_comma = false;

  public:
    Print& output;
    StreamingJson(Print& output) : output(output) {}

    void check_comma() {
      if(_needs_comma) output.print(',');
      _needs_comma = false;
    }
    void needs_comma() { _needs_comma = true; }

    void begin_dict() { output.print('{'); _needs_comma = false; }
    void end_dict()   { output.print('}'); _needs_comma = false; }
    void begin_list() { output.print('['); _needs_comma = false; }
    void end_list()   { output.print(']'); _needs_comma = false; }

    void begin_str(char quote='"') { output.print(quote); }
    void end_str(char quote='"') { output.print(quote); }

    void key(const char* str, char quote='"') {
      check_comma();
      begin_str(quote);
      output.print(str);
      end_str(quote);
      output.print(':');
    }

    // value types

    void val(const char* str, char quote='"') {
      check_comma();
      begin_str(quote);
      output.print(str);
      end_str(quote);
      needs_comma();
    }

    void val(bool b) {
       check_comma();
       output.print(b ? "true," : "false,");
       needs_comma();
    }

    // should ensure things do not behave as strings...
    template<typename V>
    void val(V i) {
       check_comma();
       output.print(i);
       needs_comma();
    }

    void null() {
       check_comma();
       output.print("null");
       needs_comma();
    }

    /// An embedded Raw json structure.
    void json(const char* str) {
      check_comma();
      output.print(str);
      needs_comma();
    }

    // Shorthands

    template<typename V> void kv(const char* _key, V _val) { key(_key); val(_val); }

    // scope handlers
  };

}