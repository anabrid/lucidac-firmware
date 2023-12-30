#ifndef UTILS_TEENSY_DCP_H
#define UTILS_TEENSY_DCP_H

#include <cstdint>
#include <cstddef>

namespace utils {

/**
 * Computes the SHA256 sum of an arbitrary message (large memory segment),
 * hardware-accelerated on the Teensy 4.
 * Outputs an uint8_t[32].
 **/
void hash_sha256(const uint8_t* msg, size_t msg_len, uint8_t* out_hash);


// note: Could expose further functions, such as prhash(out_hash, 32) for a
//       simple string representation

} // end of namespace
#endif /* UTILS_TEENSY_DCP_H */