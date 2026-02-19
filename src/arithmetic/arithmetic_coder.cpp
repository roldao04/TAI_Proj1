#include "arithmetic/arithmetic_coder.h"
#include <stdexcept>

// ============================================================================
// ArithmeticEncoder Implementation
// ============================================================================

ArithmeticEncoder::ArithmeticEncoder()
    : low(0), high(TOP_VALUE), pending_bits(0), current_byte(0), bits_in_current_byte(0) {
}

void ArithmeticEncoder::output_bit(int bit) {
    current_byte = (current_byte << 1) | bit;
    bits_in_current_byte++;

    if (bits_in_current_byte == 8) {
        output_buffer.push_back(current_byte);
        current_byte = 0;
        bits_in_current_byte = 0;
    }
}

void ArithmeticEncoder::output_bit_plus_pending(int bit) {
    output_bit(bit);
    while (pending_bits > 0) {
        output_bit(1 - bit);
        pending_bits--;
    }
}

void ArithmeticEncoder::encode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq) {
    // Update range
    uint64_t range = (uint64_t)high - (uint64_t)low + 1;
    high = low + (uint32_t)((range * cum_freq_high) / total_freq) - 1;
    low = low + (uint32_t)((range * cum_freq_low) / total_freq);

    // Renormalization
    while (true) {
        if (high < HALF) {
            output_bit_plus_pending(0);
        } else if (low >= HALF) {
            output_bit_plus_pending(1);
            low -= HALF;
            high -= HALF;
        } else if (low >= FIRST_QTR && high < THIRD_QTR) {
            pending_bits++;
            low -= FIRST_QTR;
            high -= FIRST_QTR;
        } else {
            break;
        }
        low = low << 1;
        high = (high << 1) | 1;
    }
}

void ArithmeticEncoder::finish() {
    pending_bits++;
    if (low < FIRST_QTR) {
        output_bit_plus_pending(0);
    } else {
        output_bit_plus_pending(1);
    }

    // Flush remaining bits
    if (bits_in_current_byte > 0) {
        current_byte <<= (8 - bits_in_current_byte);
        output_buffer.push_back(current_byte);
    }
}

// ============================================================================
// ArithmeticDecoder Implementation
// ============================================================================

ArithmeticDecoder::ArithmeticDecoder(const std::vector<uint8_t>& input)
    : low(0), high(TOP_VALUE), value(0), input_buffer(input),
      current_byte_index(0), bits_in_current_byte(0) {

    // Initialize value with first 32 bits
    for (uint32_t i = 0; i < CODE_VALUE_BITS; i++) {
        value = (value << 1) | input_bit();
    }
}

int ArithmeticDecoder::input_bit() {
    // Check if we need to load a new byte
    if (bits_in_current_byte == 0) {
        if (current_byte_index >= input_buffer.size()) {
            return 0;  // Return 0 if we've read all input
        }
        bits_in_current_byte = 8;
        // Note: we don't increment current_byte_index here, it will be done after reading all 8 bits
    }

    // Extract the bit from current byte (MSB first)
    int bit = (input_buffer[current_byte_index] >> (bits_in_current_byte - 1)) & 1;
    bits_in_current_byte--;

    // After reading all 8 bits from current byte, move to next byte
    if (bits_in_current_byte == 0) {
        current_byte_index++;
    }

    return bit;
}

uint32_t ArithmeticDecoder::get_current_count(uint32_t total_freq) {
    uint64_t range = (uint64_t)high - (uint64_t)low + 1;
    uint64_t scaled = ((uint64_t)(value - low) + 1) * total_freq - 1;
    uint32_t count = (uint32_t)(scaled / range);
    return count;
}

void ArithmeticDecoder::decode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq) {
    // Update range (same as encoder)
    uint64_t range = (uint64_t)high - (uint64_t)low + 1;
    high = low + (uint32_t)((range * cum_freq_high) / total_freq) - 1;
    low = low + (uint32_t)((range * cum_freq_low) / total_freq);

    // Renormalization
    while (true) {
        if (high < HALF) {
            // Do nothing
        } else if (low >= HALF) {
            low -= HALF;
            high -= HALF;
            value -= HALF;
        } else if (low >= FIRST_QTR && high < THIRD_QTR) {
            low -= FIRST_QTR;
            high -= FIRST_QTR;
            value -= FIRST_QTR;
        } else {
            break;
        }
        low = low << 1;
        high = (high << 1) | 1;
        value = (value << 1) | input_bit();
    }
}
