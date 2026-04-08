#ifndef LZP_H
#define LZP_H

#include <vector>
#include <cstdint>

/*
 * LZP (Lempel-Ziv Prediction) Preprocessor
 *
 * Removes long-range byte repetitions before BWT by predicting the next byte
 * from a 2-byte context hash table. When the prediction matches, only a flag
 * bit is needed (no literal byte). This makes BWT sorting more effective.
 *
 * Format: groups of 8 bytes → 1 flag byte + 0-8 literal bytes.
 * Flag bit = 1 means match (use prediction), 0 means literal follows.
 *
 * Used by BSC and bzip3 as a BWT preprocessor.
 */

class LZPredictor {
public:
    // Encode: returns LZP-compressed data (flag bytes + literals).
    // Returns empty vector if LZP doesn't reduce size (caller should skip).
    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input);

    // Decode: reconstructs original data from LZP stream.
    static std::vector<uint8_t> decode(const std::vector<uint8_t>& encoded, uint64_t original_size);

private:
    static const int HASH_SIZE = 65536;  // 2-byte context, no collisions
};

#endif // LZP_H
