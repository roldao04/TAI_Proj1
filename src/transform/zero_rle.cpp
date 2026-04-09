#include "transform/zero_rle.h"

/*
 * RLE0 encode:
 *   - Zero runs → RUNA/RUNB sequence (bijective base-2)
 *   - Non-zero byte b → byte b+1  (shifts ranks 1-253 to bytes 2-254)
 *   - Ranks 254/255 → escape byte 255 followed by the literal rank
 *
 * The escape prefix avoids the uint8_t overflow that previously corrupted
 * rank 255 (255+1=256→0=RUNA). Cost: 1 extra byte per rank-254/255
 * occurrence, which is negligible (these ranks are extremely rare after
 * BWT+MTF — typically <100 per MB).
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
            uint8_t rank = input[i];
            if (rank <= 253) {
                output.push_back(static_cast<uint8_t>(rank + 1));  // ranks 1-253 → bytes 2-254
            } else {
                output.push_back(255);    // escape prefix
                output.push_back(rank);   // literal 254 or 255
            }
            i++;
        }
    }

    return output;
}

/*
 * RLE0 decode:
 *   - Read leading RUNA/RUNB symbols → decode run length, emit zeros
 *   - Read next non-zero byte b:
 *       - If b == 255 (escape): next byte is the literal rank (254 or 255)
 *       - Otherwise: emit byte b-1 (reverse shift)
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

        // Emit non-zero value (if present)
        if (i < input.size()) {
            if (input[i] == 255 && i + 1 < input.size()) {
                // Escaped rank: next byte is the literal rank (254 or 255)
                i++;
                output.push_back(input[i]);
            } else {
                output.push_back(static_cast<uint8_t>(input[i] - 1));
            }
            i++;
        }
    }

    return output;
}
