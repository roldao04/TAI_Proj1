#ifndef ARITHMETIC_CODER_H
#define ARITHMETIC_CODER_H

#include <vector>
#include <cstdint>
#include <fstream>

/*
 * Arithmetic Coding Implementation
 * Based on standard arithmetic coding algorithms
 * Reference: "Introduction to Data Compression" by Khalid Sayood
 *
 * This implementation uses:
 * - 32-bit precision for low and high bounds
 * - Frequency-based probability model
 * - Bit-by-bit output with carry handling
 */

class ArithmeticEncoder {
private:
    static const uint32_t CODE_VALUE_BITS = 32;
    static const uint32_t TOP_VALUE = 0xFFFFFFFF;
    static const uint32_t FIRST_QTR = 0x40000000;
    static const uint32_t HALF = 0x80000000;
    static const uint32_t THIRD_QTR = 0xC0000000;

    uint32_t low;
    uint32_t high;
    uint32_t pending_bits;
    std::vector<uint8_t> output_buffer;
    uint8_t current_byte;
    int bits_in_current_byte;

    void output_bit(int bit);
    void output_bit_plus_pending(int bit);

public:
    ArithmeticEncoder();

    void encode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq);
    void finish();
    const std::vector<uint8_t>& get_output() const { return output_buffer; }
};

class ArithmeticDecoder {
private:
    static const uint32_t CODE_VALUE_BITS = 32;
    static const uint32_t TOP_VALUE = 0xFFFFFFFF;
    static const uint32_t FIRST_QTR = 0x40000000;
    static const uint32_t HALF = 0x80000000;
    static const uint32_t THIRD_QTR = 0xC0000000;

    uint32_t low;
    uint32_t high;
    uint32_t value;
    const std::vector<uint8_t>& input_buffer;
    size_t current_byte_index;
    int bits_in_current_byte;

    int input_bit();

public:
    ArithmeticDecoder(const std::vector<uint8_t>& input);

    uint32_t get_current_count(uint32_t total_freq);
    void decode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq);
};

#endif // ARITHMETIC_CODER_H
