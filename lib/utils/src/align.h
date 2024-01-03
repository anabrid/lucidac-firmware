#ifndef UTILS_ALIGN_H
#define UTILS_ALIGN_H

#include <cstdint>
#include <cstdlib>

namespace utils {
    // Memory alignment in a similar syntax to the linker ". = align(4)"
    // example usage: uint8_t* mem = align((uintptr_t)mem, 4)
    uintptr_t align(uintptr_t base, uint8_t exp) { return (base + exp - 1) & ~(exp - 1); }
    uint8_t*  align(uint8_t*  base, uint8_t exp) { return (uint8_t*)align((uintptr_t)base, exp); }

    // Returns the number of bytes "lost" by alignment. It is 0 <= disalignment <= exp.
    uintptr_t disalignment(uint8_t* base, uint8_t exp) { return align(base,exp) - base;  }

    /**
     * ARM32 requires callable addresses to be memory aligned. This should be ensured
     * by both the plugin linker and a properly aligned memory chunk. Returns NULLPTR if not
     * callable.
     * Furthermore, since jumping to addr results in a BLX instruction, it requires
     * bit 1 to be set for the correct thumb instructionset. This either works by +1 or &1.
     **/
    inline uint8_t* assert_callable(uint8_t* addr) {
        if(disalignment(addr, 4) != 0) return 0;
        return addr + 1;
    }

}

#endif /* UTILS_ALIGN_H */