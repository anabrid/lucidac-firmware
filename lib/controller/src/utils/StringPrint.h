// Copyright (c) 2024 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <string>
#include <Arduino.h> // Print

namespace utils {

    // This is like ArduinoStreamUtils' StringPrint (https://github.com/bblanchon/ArduinoStreamUtils?tab=readme-ov-file#writing-to-a-string)
    // but with STL strings and not Arduino Strings as target
    class StringPrint : public Print {
    public:
    StringPrint() {}

    explicit StringPrint(std::string str) : _str(str) {}

    size_t write(const uint8_t* p, size_t n) override {
        for (size_t i = 0; i < n; i++) {
        uint8_t c = p[i];
        if (c == 0)
            return i;
        write(c);
        }
        return n;
    }

    size_t write(uint8_t c) override {
        if (c == 0)
        return 0;
        _str += static_cast<char>(c);
        return 1;
    }

    const std::string& str() const {
        return _str;
    }

    void str(std::string str) {
        _str = str;
    }

    void clear() {
        _str.clear();
    }

    private:
    std::string _str;
    };

}