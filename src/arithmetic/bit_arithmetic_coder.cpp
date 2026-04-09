#include "arithmetic/bit_arithmetic_coder.h"
#include <algorithm>
#include <iostream>

// ============================================================================
// BitArithmeticEncoder Implementation - Improved 64-bit
// ============================================================================

BitArithmeticEncoder::BitArithmeticEncoder(std::vector<uint8_t>& out)
    : low(0), high(0xFFFFFFFFFFFFFFFFULL), output(out), pending_bits(0) {
    // Use full 64-bit range for maximum precision
}

void BitArithmeticEncoder::encode_bit(bool bit, double p_zero) {
    // Clamp probability to safe valid range
    // Less aggressive than 0.001-0.999, more conservative than 0.0001-0.9999
    p_zero = std::clamp(p_zero, 0.0005, 0.9995);

    // Calculate split point
    uint64_t range = high - low + 1;
    uint64_t mid = low + (uint64_t)(range * p_zero);

    // Prevent mid from being exactly at boundaries
    if (mid <= low) mid = low + 1;
    if (mid >= high) mid = high - 1;

    // Update range based on bit value
    if (bit == 0) {
        high = mid;
    } else {
        low = mid + 1;
    }

    // Renormalize when range becomes small
    renormalize();
}

void BitArithmeticEncoder::renormalize() {
    // Output when top byte matches (balance between precision and efficiency)
    while (((low ^ high) & 0xFF00000000000000ULL) == 0) {
        output.push_back((uint8_t)(high >> 56));
        low <<= 8;
        high = (high << 8) | 0xFF;
    }
}

void BitArithmeticEncoder::finish() {
    // Output remaining 8 bytes
    for (int i = 7; i >= 0; i--) {
        output.push_back((uint8_t)(high >> (i * 8)));
    }
}

// ============================================================================
// BitArithmeticDecoder Implementation - Improved 64-bit
// ============================================================================

BitArithmeticDecoder::BitArithmeticDecoder(const std::vector<uint8_t>& in)
    : low(0), high(0xFFFFFFFFFFFFFFFFULL), value(0), input(in), input_pos(0) {

    // Initialize value by reading first 8 bytes (64-bit range)
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
    // Clamp probability to same range as encoder
    p_zero = std::clamp(p_zero, 0.0005, 0.9995);

    // Calculate split point
    uint64_t range = high - low + 1;
    uint64_t mid = low + (uint64_t)(range * p_zero);

    // Prevent mid from being exactly at boundaries
    if (mid <= low) mid = low + 1;
    if (mid >= high) mid = high - 1;

    // Determine which half of range value falls into
    bool bit;
    if (value <= mid) {
        bit = 0;
        high = mid;
    } else {
        bit = 1;
        low = mid + 1;
    }

    // Renormalize (match encoder logic)
    renormalize();

    return bit;
}

void BitArithmeticDecoder::renormalize() {
    // Match encoder renormalization
    while (((low ^ high) & 0xFF00000000000000ULL) == 0) {
        low <<= 8;
        high = (high << 8) | 0xFF;
        value = (value << 8) | read_byte();
    }
}
