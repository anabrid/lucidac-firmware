
#pragma once

#include <Arduino.h>
#include <ArduinoJson.h> // just for adapter

namespace utils {

/**
 * This is a small UUID representation class. Only for holding pre-generated
 * UUIDs. NOT for generating new UUIDs. It thus has a completely different scope then
 * https://github.com/RobTillaart/UUID
 * but borrows the printing code.
 * 
 * Generation is like UUID{0x26,0x5,0x1b,0xb5,0x7c,0xb7,0x43,0xfd,0x8d,0x64,0xe2,0x4f,0xdf,0xa1,0x44,0x89}
 **/
struct UUID { // : public Printable { // no inheritance to maintain POD
public:
    uint8_t ar[16];
    // hint: Could also use something like the following if helps.
    /*
       union {
        uint8_t  bytes[16];
        uin16_t  nibbles[8];
        uint32_t quads[4];
	   } _ar;
    */


    // TODO: Should unit test whether this is correct
    void toCharArray(char *_buffer) const {
        //  process 16 bytes build up the char array.
        for (uint8_t i = 0, j = 0; i < 16; i++)
        {
            //  multiples of 4 between 8 and 20 get a -.
            //  note we are processing 2 digits in one loop.
            if ((i & 0x1) == 0)
            {
                if ((4 <= i) && (i <= 10)) 
                {
                    _buffer[j++] = '-';
                }
            }

            ////  process one byte at the time instead of a nibble
            //uint8_t nr   = i / 4;
            uint8_t xx   = ar[i];
            uint8_t ch   = xx & 0x0F;
            _buffer[j++] = (ch < 10) ? '0' + ch : ('a' - 10) + ch;

            ch = (xx >> 4) & 0x0F;
            //ar[nr] >>= 8;
            _buffer[j++] = (ch < 10) ? '0' + ch : ('a' - 10) + ch;
        }

        //  if (_upperCase)
        //  {
        //    for (int i = 0; i < 37; i++)
        //    {
        //      _buffer[i] = toUpper(_buffer[i]);
        //    }
        //  }
        _buffer[36] = 0;
    }

    String toString() const {
        char _buffer[37];
        toCharArray(_buffer);
        return String(_buffer);
    }

    size_t  printTo(Print& p) const {
        //  UUID in string format
        char  _buffer[37];
        toCharArray(_buffer);
        return p.print(_buffer);
    }

};

inline void convertToJson(const UUID& uuid, JsonVariant target) {
    for(int i=0; i<16;i++) target[i] = uuid.ar[i];
}

inline void convertFromJson(JsonVariantConst src, UUID& uuid) {
    if(src.size() != 16) return;
    for(int i=0; i<16; i++)
        uuid.ar[i] = src.as<JsonArrayConst>()[i];
}

} // namespace utils