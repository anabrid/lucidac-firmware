#include <Arduino.h>
#include <cstdint>

// One left weirdness, why is this needed? Probably for above reason.
void * operator new(size_t size); // { return malloc(size); }

struct Plugin {
  Plugin() {
    Serial.println("Hi from my Plugin class");
  }
}

extern "C" void plugin_setup() {
  Serial.println("Hello world from plugin");
}

Plugin *p;
extern "C" void plugin_entry() { p = new Plugin(); }
extern "C" void plugin_exit() { delete p; }