#include <cstddef>
#include <cstdint>
#include <cstring>

namespace utils {
    /**
     * Helpers to decode (and probably in future also encode) BASE64 encoded data.
     * This implementation works only on the stack, no allocation taken place.
     * This is certainly not the fastest implementation.
     **/
    struct base64 { // following just https://stackoverflow.com/a/6782480 but it is kind of common code
        const char *encoding_table = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        uint8_t decoding_table[256]; // yes we could precompute it in flash.
        inline base64() { for(uint8_t i = 0; i < 64; i++)  decoding_table[(unsigned char) encoding_table[i]] = i; }

        /**
         * Returns true on success.
         * Requires output to point to a buffer of size >= input_length/4*3.
         * TODO: Should not be inline.
         **/
        inline bool decode(const char* data, size_t input_length, uint8_t* output) {
            if(input_length % 4 != 0) return false;
            int output_length = input_length / 4 * 3;
            if (data[input_length - 1] == '=') output_length--;
            if (data[input_length - 2] == '=') output_length--;

            for (size_t i = 0, j = 0; i < input_length;) {
                if(data[i] != '=' && strchr(encoding_table, data[i] != NULL)) return false; // illegal character
                uint32_t a = data[i] == '=' ? 0 & i++ : decoding_table[(size_t)data[i++]],
                         b = data[i] == '=' ? 0 & i++ : decoding_table[(size_t)data[i++]],
                         c = data[i] == '=' ? 0 & i++ : decoding_table[(size_t)data[i++]],
                         d = data[i] == '=' ? 0 & i++ : decoding_table[(size_t)data[i++]],
                         triple = (a << 3 * 6) + (b << 2 * 6) + (c << 1 * 6) + (d << 0 * 6);
                if (j < output_length) output[j++] = (triple >> 2 * 8) & 0xFF;
                if (j < output_length) output[j++] = (triple >> 1 * 8) & 0xFF;
                if (j < output_length) output[j++] = (triple >> 0 * 8) & 0xFF;
            }
            return true;
        }
    };
}