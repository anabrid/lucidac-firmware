
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h> // just for adapter
#include <string>
#include <cstdlib>

namespace utils {

/**
 * This is a small UUID representation class. Only for holding pre-generated
 * UUIDs. NOT for generating new UUIDs. It thus has a completely different scope then
 * https://github.com/RobTillaart/UUID
 * but borrows the printing code.
 * 
 * Generation is like UUID{0x26,0x5,0x1b,0xb5,0x7c,0xb7,0x43,0xfd,0x8d,0x64,0xe2,0x4f,0xdf,0xa1,0x44,0x89}
 * or like UUID::from_String("2d2f7585-c53c-465b-8844-1294c52dc917").
 **/
struct UUID { // : public Printable { // no inheritance to maintain POD
public:
    uint8_t ar[16] = {0};
    // hint: Could also use something like the following if helps.
    /*
       union {
        uint8_t  bytes[16];
        uin16_t  nibbles[8];
        uint32_t quads[4];
	   } _ar;
    */

   /**
    * Checks if UUID is all zeros, can be used for detecting illegal values
    * and failed conversions
    **/
   bool allZero() {
      for(size_t i=0; i<16; i++) if(ar[i] != 0) return false;
      return true;
   }

    bool operator==(const UUID& other) {
        for(int i=0; i<16; i++) if(other.ar[i] != ar[i]) return false;
        return true;
    }

    void toCharArray(char *_buffer) const;
    String toString() const;
    size_t printTo(Print& p) const;

    /// Parses standard UUID representation such as "2d2f7585-c53c-465b-8844-1294c52dc917",
    /// returns an allZero instance in case of error.
    static UUID fromString(const char* str);

    /// Provides only the Array representation
    void toJson(JsonArray target) const {
        for(int i=0; i<16;i++) target[i] = ar[i];
    }
    
    void fromJson(JsonArrayConst src) {
        if(src.size() != 16) return;
        for(int i=0; i<16; i++) ar[i] = src[i];
    }

    /// Can read both string and array representation
    void fromJson(JsonVariantConst src) {
        if(src.is<const char*>()) {
            fromString(src.as<const char*>());
        } else {
            fromJson(src.as<JsonArrayConst>());
        }
    }
};

inline void convertToJson(const UUID& uuid, JsonVariant target) {
    uuid.toJson(target.to<JsonArray>());
}

inline void convertFromJson(JsonVariantConst src, UUID& uuid) {
    uuid.fromJson(src);
}

} // namespace utils