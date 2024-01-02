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

}

#endif /* UTILS_HASH_FLASH_H */