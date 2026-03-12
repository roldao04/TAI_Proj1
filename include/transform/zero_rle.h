#ifndef ZERO_RLE_H
#define ZERO_RLE_H

#include <cstddef>
#include <cstdint>
#include <vector>

class ZeroRunLengthEncoder {
public:
    static constexpr uint8_t MARKER = 255;
    static constexpr size_t DEFAULT_MIN_RUN_LENGTH = 4;

    static std::vector<uint8_t> encode(const std::vector<uint8_t>& input,
                                       size_t min_run_length = DEFAULT_MIN_RUN_LENGTH);
    static std::vector<uint8_t> decode(const std::vector<uint8_t>& input);
};

#endif // ZERO_RLE_H
