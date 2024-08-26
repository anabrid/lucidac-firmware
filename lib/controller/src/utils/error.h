// Copyright (c) 2023 anabrid GmbH
// Contact: https://www.anabrid.com/licensing/
// SPDX-License-Identifier: MIT OR GPL-2.0-or-later

#pragma once

#include <string>
#include <cstdarg>

namespace utils {

    template<typename... Args>
    std::string small_sprintf(const char* format, Args... args) {
        constexpr unsigned int bufsize=1000;
        char buf[bufsize];
        snprintf(buf, bufsize, format, args...);
        return buf;
    }

    /**
     * A recoverable error, inspired from https://abseil.io/docs/cpp/guides/status
     * and https://github.com/abseil/abseil-cpp/blob/master/absl/status/status.h
     * 
     * This class should be used as immutable.
     * 
     * For making the success path cheap, use status::success.
     **/
    class status {
    public:
        int code;
        std::string msg;

        status(int code, std::string msg) : code(code), msg(msg) {}

        status(int code) : code(code) {}

        status(std::string msg) : code(-1), msg(msg) {} ///< Generic failure (without code)

        status() : code(0) {} ///< Generic Success

        status(const char *msg) : status(std::string(msg)) {}

        /// Usage like status("Foo %d bar %s baz", 3, "bling");
        template<typename... Args>
        status(const char* format, Args... args) : status(small_sprintf(format, args...)) {}

        bool is_ok() const { return code == 0; }
        operator bool() const { return is_ok(); }

        /// Syntactic sugar
        static status success() { return status(0); }
    };
} // end of namespace

