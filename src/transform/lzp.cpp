#include "transform/lzp.h"
#include <cstring>
#include <algorithm>

std::vector<uint8_t> LZPredictor::encode(const std::vector<uint8_t>& input) {
    if (input.size() < 64) return {};  // too small

    std::vector<uint8_t> output;
    output.reserve(input.size());

    uint8_t table[HASH_SIZE];
    std::memset(table, 0, sizeof(table));
    uint8_t prev0 = 0, prev1 = 0;

    size_t i = 0;
    while (i < input.size()) {
        size_t flag_pos = output.size();
        output.push_back(0);  // placeholder for flag byte

        uint8_t flag = 0;
        int bits = static_cast<int>(std::min<size_t>(8, input.size() - i));
        for (int b = 0; b < bits; b++) {
            uint16_t ctx = ((uint16_t)prev1 << 8) | prev0;
            uint8_t prediction = table[ctx];
            uint8_t byte = input[i];

            if (byte == prediction) {
                flag |= (1 << (7 - b));
            } else {
                output.push_back(byte);
            }

            table[ctx] = byte;
            prev1 = prev0;
            prev0 = byte;
            i++;
        }

        output[flag_pos] = flag;
    }

    // Only return LZP output if it's actually smaller
    if (output.size() >= input.size()) return {};

    return output;
}

std::vector<uint8_t> LZPredictor::decode(const std::vector<uint8_t>& encoded, uint64_t original_size) {
    std::vector<uint8_t> output;
    output.reserve(original_size);

    uint8_t table[HASH_SIZE];
    std::memset(table, 0, sizeof(table));
    uint8_t prev0 = 0, prev1 = 0;

    size_t pos = 0;
    while (output.size() < original_size && pos < encoded.size()) {
        uint8_t flag = encoded[pos++];

        int bits = static_cast<int>(std::min<size_t>(8, original_size - output.size()));
        for (int b = 0; b < bits; b++) {
            uint16_t ctx = ((uint16_t)prev1 << 8) | prev0;
            uint8_t byte;

            if (flag & (1 << (7 - b))) {
                byte = table[ctx];
            } else {
                if (pos >= encoded.size()) break;
                byte = encoded[pos++];
            }

            table[ctx] = byte;
            output.push_back(byte);
            prev1 = prev0;
            prev0 = byte;
        }
    }

    return output;
}
