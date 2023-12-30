#ifndef UTILS_HASH_FLASH_H
#define UTILS_HASH_FLASH_H

#include <array>
#include <string>
#include <cstdio> // sprintf
#include "dcp.h" // utils::hash_sha256

// defined in the Teensy default linker script imxrt1062_t41.ld
extern unsigned long _flashimagelen;
// typically hardcoded everywhere, but see also the linker script.
constexpr unsigned long flash_origin = 0x60000000;

namespace utils {
    using sha256_t = std::array<uint8_t, 32>;

    /**
     * Returns the number of bytes of the flash image. This is identical to the output of
     * 
     * shell> objcopy --input-target=ihex --output-target=binary firmware.hex firmware.bin
     * shell> ls -l firmware.bin
     *
     * i.e. the file size of the firmware image in binary format.
     * Note that the linker storage cell actually holds data, cf.
     * https://sourceware.org/binutils/docs/ld/Source-Code-Reference.html
     **/
    inline uint32_t flashimagelen() { return (uint32_t)&_flashimagelen; }

    /**
     * Computes a SHA256 hash of the program image stored on Flash. This is identical
     * to the output of
     * 
     * shell> objcopy --input-target=ihex --output-target=binary firmware.hex firmware.bin
     * shell> sha256sum firmware.bin
     * 
     * The computation takes 3383us. If you use the output in networking requests, it is
     * fast enough to compute it on need, no caching neccessary.
     **/
    inline sha256_t hash_flash_sha256() {
        auto *flash = (const uint8_t*) flash_origin;
        sha256_t checksum;
        utils::hash_sha256(flash, flashimagelen(), checksum.data());
        return checksum;
    }

    inline std::string sha256_to_string(const sha256_t& hash) {
        char out64[64+1]; // 0-terminated
        for(int pin=0; pin<32; pin++) {
            int pout = 2*pin;
            std::sprintf(out64+pout, "%02x", hash[pin]);
        }
        return std::string(out64);
    }
}

#endif /* UTILS_HASH_FLASH_H */