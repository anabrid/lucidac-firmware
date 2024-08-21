#include <Arduino.h>

namespace utils {

class SerialLineReader {
public:
  static constexpr int serial_buffer_size = 4096;
  char serial_buffer[serial_buffer_size];
  int serial_buffer_pos = 0;

  bool echo = true; ///< whether to give immediate feedback on typing or not.

  char* line_available() {
    if(Serial.available() > 0) {
      char in = Serial.read();

      if(echo) {
        Serial.print(in); Serial.flush();
      }

      if(in == '\n') {
        serial_buffer[serial_buffer_pos] = '\0';
        serial_buffer_pos = 0;
        return serial_buffer;
      } else if (serial_buffer_pos < serial_buffer_size - 1) {  // Avoid buffer overflow
        serial_buffer[serial_buffer_pos++] = in;
      } else {
        // buffer is full, flush it anyway.
        serial_buffer[serial_buffer_pos-2] = '\n';
        serial_buffer[serial_buffer_pos-1] = '\0';
        serial_buffer_pos = 0;
        return serial_buffer;
      }
    }
    return NULL;
  }
};

}