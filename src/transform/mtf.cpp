#include "transform/mtf.h"

#include <algorithm>
#include <array>
#include <cstring>

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
    if (rank == 0) return;

    uint8_t symbol = symbols[rank];
    // Hardware-optimized memory move (SIMD) instead of element-by-element loop
    std::memmove(&symbols[1], &symbols[0], rank);
    symbols[0] = symbol;
    // Update positions for shifted symbols
    for (uint8_t idx = rank; idx > 0; idx--) {
        positions[symbols[idx]] = idx;
    }
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

// Inverse transform: no positions array needed (only read symbols[rank])
std::vector<uint8_t> inverse_transform_range(const std::vector<uint8_t>& input,
                                             size_t start,
                                             size_t end) {
    std::vector<uint8_t> output;
    output.reserve(end - start);

    std::array<uint8_t, 256> symbols;
    for (size_t i = 0; i < 256; i++) symbols[i] = static_cast<uint8_t>(i);

    for (size_t idx = start; idx < end; idx++) {
        uint8_t rank = input[idx];
        uint8_t symbol = symbols[rank];
        output.push_back(symbol);
        if (rank > 0) {
            std::memmove(&symbols[1], &symbols[0], rank);
            symbols[0] = symbol;
        }
    }

    return output;
}

} // namespace

std::vector<uint8_t> MoveToFront::wfc_transform(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    output.reserve(input.size());

    std::array<uint8_t, 256> symbols;
    std::array<uint8_t, 256> positions;
    std::array<uint32_t, 256> counts;
    for (int i = 0; i < 256; i++) {
        symbols[i]   = static_cast<uint8_t>(i);
        positions[i] = static_cast<uint8_t>(i);
        counts[i]    = 0;
    }

    for (uint8_t sym : input) {
        uint8_t rank = positions[sym];
        output.push_back(rank);

        counts[sym]++;

        // Bubble up while this symbol's count exceeds left neighbor's
        while (rank > 0) {
            uint8_t left = symbols[rank - 1];
            if (counts[sym] <= counts[left]) break;
            // Swap sym and left neighbor
            symbols[rank]    = left;
            symbols[rank - 1] = sym;
            positions[left]  = rank;
            rank--;
        }
        positions[sym] = rank;
    }

    return output;
}

std::vector<uint8_t> MoveToFront::wfc_inverse_transform(const std::vector<uint8_t>& input) {
    std::vector<uint8_t> output;
    output.reserve(input.size());

    std::array<uint8_t, 256> symbols;
    std::array<uint32_t, 256> counts;
    for (int i = 0; i < 256; i++) {
        symbols[i] = static_cast<uint8_t>(i);
        counts[i]  = 0;
    }

    for (uint8_t rank : input) {
        uint8_t sym = symbols[rank];
        output.push_back(sym);

        counts[sym]++;

        // Same bubble-up as forward
        while (rank > 0) {
            uint8_t left = symbols[rank - 1];
            if (counts[sym] <= counts[left]) break;
            symbols[rank]    = left;
            rank--;
        }
        symbols[rank] = sym;
    }

    return output;
}

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
