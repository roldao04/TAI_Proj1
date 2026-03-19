#include "transform/zero_rle.h"

/*
 * RLE0 encode:
 *   - Zero runs → RUNA/RUNB sequence (bijective base-2)
 *   - Non-zero byte b → byte b+1  (shifts ranks 1-254 to bytes 2-255)
 *
 * Note: rank 255 maps to byte 256 which overflows uint8_t to 0 (= RUNA),
 * corrupting the stream. Callers must skip ZRLE for blocks containing MTF
 * rank 255 (detected via has_rank255 guard in compressor.cpp).
 */
std::vector<uint8_t> ZeroRunLengthEncoder::encode(const std::vector<uint8_t>& input,
                                                   size_t /* unused */) {
    std::vector<uint8_t> output;
    output.reserve(input.size());

    size_t i = 0;
    while (i < input.size()) {
        if (input[i] == 0) {
            // Count zero run
            size_t run = 0;
            while (i < input.size() && input[i] == 0) { run++; i++; }

            // Encode run length in bijective base-2, LSB first
            // RUNA represents 1 at position k, RUNB represents 2 at position k
            size_t n = run;
            while (n > 0) {
                if (n & 1) { output.push_back(RUNA); n = (n - 1) >> 1; }
                else       { output.push_back(RUNB); n = (n - 2) >> 1; }
            }
        } else {
            // Shift non-zero rank up by 1: rank 1→2, rank 254→255
            output.push_back(static_cast<uint8_t>(input[i] + 1));
            i++;
        }
    }

    return output;
}

/*
 * RLE0 decode:
 *   - Read leading RUNA/RUNB symbols → decode run length, emit zeros
 *   - Read next non-zero byte b → emit byte b-1 (reverse shift)
 */
std::vector<uint8_t> ZeroRunLengthEncoder::decode(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    output.reserve(input.size());

    size_t i = 0;
    while (i < input.size()) {
        // Accumulate RUNA/RUNB run
        uint32_t n = 0, base = 1;
        while (i < input.size() && (input[i] == RUNA || input[i] == RUNB)) {
            if (input[i] == RUNA) n += base;
            else                   n += base * 2;
            base <<= 1;
            i++;
        }
        // Emit n zeros
        output.insert(output.end(), n, 0);

        // Emit shifted non-zero value (if present)
        if (i < input.size()) {
            output.push_back(static_cast<uint8_t>(input[i] - 1));
            i++;
        }
    }

    return output;
}
