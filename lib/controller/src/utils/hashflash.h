#ifndef UTILS_HASH_FLASH_H
#define UTILS_HASH_FLASH_H

#include "utils/dcp.h" // utils::hash_sha256
#include <ArduinoJson.h>
#include <array>
#include <cstdio> // sprintf
#include <string>

// defined in the Teensy default linker script imxrt1062_t41.ld
extern unsigned long _flashimagelen;
// typically hardcoded everywhere, but see also the linker script.
constexpr unsigned long flash_origin = 0x60000000;

namespace loader {

struct flashimage {
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
  static uint32_t len() { return (uint32_t)&_flashimagelen; };

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
  static utils::sha256 sha256sum() {
    auto *flash = (const uint8_t *)flash_origin;
    return utils::sha256(flash, len());
    // sha256_t checksum;
    // utils::hash_sha256(flash, flashimagelen(), checksum.data());
    // return checksum;
  };

  static void toJson(JsonVariant target) {
    target["size"] = len();
    target["sha256sum"] = sha256sum().to_string();
  }
};

inline void convertToJson(const flashimage &src, JsonVariant info) { src.toJson(info); }

} // namespace loader

#endif /* UTILS_HASH_FLASH_H */
