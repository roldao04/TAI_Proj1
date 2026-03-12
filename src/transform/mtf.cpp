#include "transform/mtf.h"

#include <algorithm>
#include <array>

namespace {

void reset_mtf_state(std::array<uint8_t, 256>& symbols,
                     std::array<uint8_t, 256>& positions) {
    for (size_t i = 0; i < symbols.size(); i++) {
        symbols[i] = static_cast<uint8_t>(i);
        positions[i] = static_cast<uint8_t>(i);
    }
}

void move_symbol_to_front(std::array<uint8_t, 256>& symbols,
                          std::array<uint8_t, 256>& positions,
                          uint8_t rank) {
    if (rank == 0) {
        return;
    }

    uint8_t symbol = symbols[rank];
    for (size_t idx = rank; idx > 0; idx--) {
        symbols[idx] = symbols[idx - 1];
        positions[symbols[idx]] = static_cast<uint8_t>(idx);
    }

    symbols[0] = symbol;
    positions[symbol] = 0;
}

std::vector<uint8_t> transform_range(const std::vector<uint8_t>& input,
                                     size_t start,
                                     size_t end) {
    std::vector<uint8_t> output;
    output.reserve(end - start);

    std::array<uint8_t, 256> symbols;
    std::array<uint8_t, 256> positions;
    reset_mtf_state(symbols, positions);

    for (size_t idx = start; idx < end; idx++) {
        uint8_t symbol = input[idx];
        uint8_t rank = positions[symbol];
        output.push_back(rank);
        move_symbol_to_front(symbols, positions, rank);
    }

    return output;
}

std::vector<uint8_t> inverse_transform_range(const std::vector<uint8_t>& input,
                                             size_t start,
                                             size_t end) {
    std::vector<uint8_t> output;
    output.reserve(end - start);

    std::array<uint8_t, 256> symbols;
    std::array<uint8_t, 256> positions;
    reset_mtf_state(symbols, positions);

    for (size_t idx = start; idx < end; idx++) {
        uint8_t rank = input[idx];
        uint8_t symbol = symbols[rank];
        output.push_back(symbol);
        move_symbol_to_front(symbols, positions, rank);
    }

    return output;
}

} // namespace

std::vector<uint8_t> MoveToFront::transform(const std::vector<uint8_t>& input) {
    return transform_range(input, 0, input.size());
}

std::vector<uint8_t> MoveToFront::inverse_transform(const std::vector<uint8_t>& input) {
    return inverse_transform_range(input, 0, input.size());
}

std::vector<uint8_t> MoveToFront::transform_blocks(const std::vector<uint8_t>& input,
                                                   size_t block_size) {
    if (input.empty()) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> output;
    output.reserve(input.size());

    for (size_t start = 0; start < input.size(); start += block_size) {
        size_t end = std::min(start + block_size, input.size());
        std::vector<uint8_t> block_output = transform_range(input, start, end);
        output.insert(output.end(), block_output.begin(), block_output.end());
    }

    return output;
}

std::vector<uint8_t> MoveToFront::inverse_transform_blocks(const std::vector<uint8_t>& input,
                                                           size_t block_size) {
    if (input.empty()) {
        return std::vector<uint8_t>();
    }

    std::vector<uint8_t> output;
    output.reserve(input.size());

    for (size_t start = 0; start < input.size(); start += block_size) {
        size_t end = std::min(start + block_size, input.size());
        std::vector<uint8_t> block_output = inverse_transform_range(input, start, end);
        output.insert(output.end(), block_output.begin(), block_output.end());
    }

    return output;
}
