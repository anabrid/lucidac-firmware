#ifndef UTILS_TEENSY_DCP_H
#define UTILS_TEENSY_DCP_H

#include <cstdint>
#include <cstddef>
#include <string>
#include <array>

namespace utils {
    using sha256_t = std::array<uint8_t, 32>;

    /**
     * Computes the SHA256 sum of an arbitrary message (large memory segment),
     * hardware-accelerated on the Teensy 4.
     * Outputs an uint8_t[32].
     **/
    void hash_sha256(const uint8_t* msg, size_t msg_len, uint8_t* out_hash);

    inline std::string sha256_to_string(const utils::sha256_t& hash) {
        char out64[64+1]; // 0-terminated
        for(int pin=0; pin<32; pin++) {
            int pout = 2*pin;
            std::sprintf(out64+pout, "%02x", hash[pin]);
        }
        return std::string(out64);
    }

    // convenience
    struct sha256 {
        utils::sha256_t checksum;
        sha256(const uint8_t* msg, size_t msg_len) { hash_sha256(msg, msg_len, checksum.data()); }
        std::string to_string() const { return sha256_to_string(checksum); }
        std::string short_string() const { return to_string().substr(0,7); }
    };

    // allow kind of git-like hash comparison
    inline bool sha256_test_short(std::string a, std::string b) {
        if(a.length() == 7 && b.length() >= 7) return a == b.substr(0,7);
        if(a.length() == 64 && b.length() == 64) return a == b;
        return false;
    }

    // note: Could expose further functions, such as prhash(out_hash, 32) for a
    //       simple string representation

} // end of namespace
#endif /* UTILS_TEENSY_DCP_H */