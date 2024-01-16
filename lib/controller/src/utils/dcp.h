#ifndef UTILS_TEENSY_DCP_H
#define UTILS_TEENSY_DCP_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <array>
#include <stdlib.h>

namespace utils {
    using sha256_t = std::array<uint8_t, 32>;

    /**
     * Computes the SHA256 sum of an arbitrary message (large memory segment),
     * hardware-accelerated on the Teensy 4.
     * Outputs an uint8_t[32].
     **/
    void hash_sha256(const uint8_t* msg, size_t msg_len, uint8_t* out_hash);

    inline sha256_t hash_sha256(const uint8_t* msg, size_t msg_len) {
        sha256_t ret; hash_sha256(msg, msg_len, ret.data()); return ret;
    }

    inline std::string sha256_to_string(const utils::sha256_t& hash) {
        char out64[64+1]; // 0-terminated
        for(int pin=0; pin<32; pin++) {
            int pout = 2*pin;
            std::sprintf(out64+pout, "%02x", hash[pin]);
        }
        return std::string(out64);
    }

    inline sha256_t parse_sha256(const std::string& hash) {
        sha256_t ret; char stroct[3]; stroct[2] = '\0';
        for(int i=0; i<32; i++) {
            stroct[0] = hash[2*i]; stroct[1] = hash[2*i+1];
            ret[i] = strtoul(stroct, NULL, 16);
        }
        return ret;
    }

    // convenience
    struct sha256 {
        utils::sha256_t checksum;
        sha256(const uint8_t* msg, size_t msg_len) { hash_sha256(msg, msg_len, checksum.data()); }
        std::string to_string() const { return sha256_to_string(checksum); }
        std::string short_string() const { return to_string().substr(0,7); }
    };

    // compares two hashes, where any kind of abbreviation is allowed, in particular the famous
    // "git short hashes" which are only the first 7 digits. To make this useful, the shorter
    // has should not be too short.
    inline bool sha256_test_short(std::string a, std::string b) {
        auto& longer  = a.length() > b.length() ? a : b,
              shorter = a.length() > b.length() ? b : a;
        return shorter == longer.substr(0, shorter.length());
    }

    // note: Could expose further functions, such as prhash(out_hash, 32) for a
    //       simple string representation

} // end of namespace
#endif /* UTILS_TEENSY_DCP_H */