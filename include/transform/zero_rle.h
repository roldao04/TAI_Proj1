#ifndef ZERO_RLE_H
#define ZERO_RLE_H

#include <cstddef>
#include <cstdint>
#include <vector>

/*
 * RLE0: bzip2-style zero-run encoding.
 *
 * Replaces runs of zeros in MTF output with RUNA/RUNB sequences.
 * Non-zero MTF ranks are shifted up by 1 (rank r → byte r+1) to free
 * byte values 0 and 1 for RUNA/RUNB.
 *
 * Run length encoding (bijective base-2, LSB first):
 *   N=1 → [RUNA]           N=2 → [RUNB]
 *   N=3 → [RUNA,RUNA]      N=4 → [RUNB,RUNA]
 *   N=5 → [RUNA,RUNB]      N=6 → [RUNB,RUNB]
 *
 * Decode: n = sum_i(sym_i * 2^i) where RUNA=1, RUNB=2, LSB first.
 *
 * Advantage over marker-based ZRLE:
 *   - No marker overhead for single/short zero runs (very common in BWT+MTF output)
 *   - Long runs cost more symbols but are nearly free under Order-1
 *     (P(RUNA|RUNA) ≈ 95%+ after BWT+MTF, so each extra symbol ≈ 0.07 bits)
 *   - MTF rank 255 → byte 256: unsupported. In practice impossible after BWT.
 */

class ZeroRunLengthEncoder {
public:
    static constexpr uint8_t RUNA = 0;
    static constexpr uint8_t RUNB = 1;

    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input, size_t = 0);
    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);
};

#endif // ZERO_RLE_H
