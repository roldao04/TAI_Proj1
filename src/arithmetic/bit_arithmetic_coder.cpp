#include "arithmetic/bit_arithmetic_coder.h"
#include <algorithm>
#include <iostream>

// ============================================================================
// BitArithmeticEncoder Implementation
// ============================================================================

BitArithmeticEncoder::BitArithmeticEncoder(std::vector<uint8_t>& out)
    : low(0), high(0xFFFFFFFFFFFFFFFFULL), output(out), pending_bits(0) {
}

void BitArithmeticEncoder::encode_bit(bool bit, double p_zero) {
    // Clamp probability to valid range
    p_zero = std::clamp(p_zero, 0.0001, 0.9999);

    // Calculate split point
    uint64_t range = high - low + 1;
    uint64_t mid = low + (uint64_t)(range * p_zero);

    // Update range based on bit value
    if (bit == 0) {
        high = mid;
    } else {
        low = mid + 1;
    }

    // Renormalize when range becomes too small
    renormalize();
}

void BitArithmeticEncoder::renormalize() {
    // Output bytes when the leading bytes of low and high match
    while (((low ^ high) & 0xFF00000000000000ULL) == 0) {
        output.push_back((uint8_t)(high >> 56));
        low <<= 8;
        high = (high << 8) | 0xFF;
    }
}

void BitArithmeticEncoder::finish() {
    // Output remaining bytes
    for (int i = 7; i >= 0; i--) {
        output.push_back((uint8_t)(high >> (i * 8)));
    }
}

// ============================================================================
// BitArithmeticDecoder Implementation
// ============================================================================

BitArithmeticDecoder::BitArithmeticDecoder(const std::vector<uint8_t>& in)
    : low(0), high(0xFFFFFFFFFFFFFFFFULL), value(0), input(in), input_pos(0) {

    // Initialize value by reading first 8 bytes
    for (int i = 0; i < 8 && input_pos < input.size(); i++) {
        value = (value << 8) | read_byte();
    }
}

uint8_t BitArithmeticDecoder::read_byte() {
    if (input_pos < input.size()) {
        return input[input_pos++];
    }
    return 0;  // Return 0 if we've read past the end
}

bool BitArithmeticDecoder::decode_bit(double p_zero) {
    // Clamp probability to valid range
    p_zero = std::clamp(p_zero, 0.0001, 0.9999);

    // Calculate split point
    uint64_t range = high - low + 1;
    uint64_t mid = low + (uint64_t)(range * p_zero);

    // Determine which half of range value falls into
    bool bit;
    if (value <= mid) {
        bit = 0;
        high = mid;
    } else {
        bit = 1;
        low = mid + 1;
    }

    // Renormalize
    renormalize();

    return bit;
}

void BitArithmeticDecoder::renormalize() {
    // Read more bytes when range narrows
    while (((low ^ high) & 0xFF00000000000000ULL) == 0) {
        low <<= 8;
        high = (high << 8) | 0xFF;
        value = (value << 8) | read_byte();
    }
}
