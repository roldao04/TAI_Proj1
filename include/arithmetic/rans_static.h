#ifndef RANS_STATIC_H
#define RANS_STATIC_H

/*
 * RansStaticCoder — rANS wrapper for Order-0 static compression.
 *
 * Entropy coder: ryg's rANS (rans_byte.h).
 * Unlike Schindler's range coder, rANS decode is division-free:
 *   cum = rans & mask        (bitmask)
 *   symbol = table[cum]      (array lookup)
 *   rans = freq * (rans >> SCALE_BITS) + (rans & mask) - start
 *
 * Constraint: all symbol frequencies must sum to exactly (1 << SCALE_BITS).
 * This is a fixed static model only; adaptive models are incompatible.
 *
 * File format (after the 0x08 model-type byte and 8-byte original_size):
 *   scale_bits  : 1 byte   (always 14 currently)
 *   scaled_freq : 257 × 2 bytes uint16_t  (symbols 0-255 + EOF=256)
 *   rans_stream : remainder of file (bytes flushed by encoder)
 */

#include <cstdint>
#include <vector>
#include <array>

class RansStaticCoder {
public:
    static constexpr int SCALE_BITS = 14;
    static constexpr int FREQ_SUM   = 1 << SCALE_BITS;  // 16384

    // Symbol alphabet: bytes 0-255 + EOF at index 256
    static constexpr int ALPHABET   = 257;
    static constexpr int EOF_SYMBOL = 256;

    // ── Encoding ─────────────────────────────────────────────────────────

    // Build the encode table (RansSymbol start/freq) from raw frequency counts.
    // Frequencies need not sum to FREQ_SUM; they will be scaled internally.
    // Returns the scaled frequencies (for writing to the compressed file).
    std::array<uint16_t, ALPHABET> build_encode_table(
        const std::array<uint32_t, ALPHABET>& raw_freq);

    // Encode the data using the table built by build_encode_table().
    // Note: rANS encodes symbols in reverse order.
    // Returns the compressed byte stream.
    std::vector<uint8_t> encode(const std::vector<uint8_t>& data);

    // ── Decoding ─────────────────────────────────────────────────────────

    // Load scaled frequencies from the compressed stream.
    // Builds the decode lookup table (alias table over FREQ_SUM slots).
    void build_decode_table(const std::array<uint16_t, ALPHABET>& scaled_freq);

    // Decode exactly original_size symbols from rans_stream.
    std::vector<uint8_t> decode(const std::vector<uint8_t>& rans_stream,
                                 uint64_t original_size);

private:
    // Per-symbol encode info
    struct EncSym {
        uint32_t start;
        uint32_t freq;
    };
    std::array<EncSym, ALPHABET> enc_table_;

    // Decode lookup: for each of the FREQ_SUM slots, which symbol owns it?
    struct DecEntry {
        uint8_t  symbol;  // 0-255 or 256=EOF (stored as uint16_t below)
        uint16_t sym16;   // same, fits EOF
        uint32_t start;
        uint32_t freq;
    };
    std::vector<DecEntry> dec_table_;  // size = FREQ_SUM

    uint32_t scale_bits_ = SCALE_BITS;
};

#endif // RANS_STATIC_H
