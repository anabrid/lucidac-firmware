


#pragma once

#include <Arduino.h>
#include "Printable.h"

namespace utils {

/**
 * Semantic Versioning. Usage like
 * Version{ 0, 0, 1}
 **/
struct Version { // : public Printable { // no inheritance to maintain POD
    uint8_t major, minor, patch;

    uint8_t at(uint8_t i) const {
        if(i==0) return major;
        if(i==1) return minor;
        if(i==2) return patch;
        else     return 255;   // missing algebraic type "Maybe[uint8_t] ...
    }

    String toString() const {
        return String(major) + "." + String(minor) + "." + String(patch);;
    }

    size_t printTo(Print& p) const {
        return p.print(toString());
    }

};

}