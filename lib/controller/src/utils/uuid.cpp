#include "utils/uuid.h"
#include "uuid.h"

// TODO: Should unit test whether this is correct
void utils::UUID::toCharArray(char *_buffer) const {
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

String utils::UUID::toString() const {
  char _buffer[37];
  toCharArray(_buffer);
  return String(_buffer);
}

size_t utils::UUID::printTo(Print &p) const {
    //  UUID in string format
    char  _buffer[37];
    toCharArray(_buffer);
    return p.print(_buffer);
}

uint8_t chartoi(char X) {
    if('0' <= X && X <= '9') return X - '0';
    if('a' <= X && X <= 'f') return X - 'a' + 10;
    if('A' <= X && X <= 'F') return X - 'A' + 10;
    return 0;
}

// TODO: Should unit test whether this is correct
utils::UUID utils::UUID::fromString(const char *str) {
    UUID data; // all zero by default

    if(strlen(str) != 36) return data;

    // implementation follows https://github.com/mariusbancila/stduuid/blob/3afe7193facd5d674de709fccc44d5055e144d7a/include/uuid.h#L503
    bool firstDigit = true;
    size_t index = 0;
    for(size_t i = 0; i < 36; i++) {
        if(str[i] == '-') continue;
        if(firstDigit) {
            data.ar[index] = chartoi(str[i]) << 4;
            firstDigit = false;
        } else {
            data.ar[index] |= chartoi(str[i]);
            index++;
            firstDigit = true;
        }
    }
    return data;
}
