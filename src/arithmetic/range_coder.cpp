#include "arithmetic/range_coder.h"
#include <stdexcept>

/*
 * Range Coder Implementation
 * Based on Michael Schindler's range coder (rangecod.cpp version 1.3)
 *
 * Original Copyright (c) Michael Schindler, 1997-2000
 * Adapted for TAI Project #1 with std::vector I/O integration
 *
 * This preserves Schindler's core algorithm while adapting I/O
 * to work with modern C++ containers.
 */

// Constants from Schindler's implementation
#define CODE_BITS 32
#define Top_value ((code_value)1 << (CODE_BITS-1))
#define SHIFT_BITS (CODE_BITS - 9)
#define EXTRA_BITS ((CODE_BITS-2) % 8 + 1)
#define Bottom_value (Top_value >> 8)

// ============================================================================
// RangeEncoder Implementation
// ============================================================================

RangeEncoder::RangeEncoder() {
    // Initialize encoder with header byte 0
    start_encoding_internal(0, 0);
}

void RangeEncoder::output_byte(uint8_t byte) {
    output_buffer.push_back(byte);
}

void RangeEncoder::start_encoding_internal(char c, int initlength) {
    rc.low = 0;                 // Full code range
    rc.range = Top_value;
    rc.buffer = c;
    rc.help = 0;                // No bytes to follow
    rc.bytecount = initlength;
}

// Schindler's normalization - outputs bytes when range gets too small
void RangeEncoder::enc_normalize() {
    while (rc.range <= Bottom_value) {  // do we need renormalisation?
        if (rc.low < (code_value)0xff << SHIFT_BITS) {  // no carry possible --> output
            output_byte(rc.buffer);
            for (; rc.help; rc.help--)
                output_byte(0xff);
            rc.buffer = (unsigned char)(rc.low >> SHIFT_BITS);
        } else if (rc.low & Top_value) {  // carry now, no future carry
            output_byte(rc.buffer + 1);
            for (; rc.help; rc.help--)
                output_byte(0);
            rc.buffer = (unsigned char)(rc.low >> SHIFT_BITS);
        } else {  // passes on a potential carry
            rc.help++;
        }
        rc.range <<= 8;
        rc.low = (rc.low << 8) & (Top_value - 1);
        rc.bytecount++;
    }
}

// Schindler's encode_freq - core encoding function
void RangeEncoder::encode_freq_internal(freq sy_f, freq lt_f, freq tot_f) {
    code_value r, tmp;
    enc_normalize();
    r = rc.range / tot_f;
    tmp = r * lt_f;
    rc.low += tmp;
    if (lt_f + sy_f < tot_f)
        rc.range = r * sy_f;
    else
        rc.range -= tmp;
}

// Public API wrapper
void RangeEncoder::encode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq) {
    freq sy_f = cum_freq_high - cum_freq_low;
    freq lt_f = cum_freq_low;
    freq tot_f = total_freq;
    encode_freq_internal(sy_f, lt_f, tot_f);
}

// Schindler's done_encoding - finish and flush
uint32_t RangeEncoder::done_encoding_internal() {
    uint32_t tmp;
    enc_normalize();  // now we have a normalized state
    rc.bytecount += 5;
    if ((rc.low & (Bottom_value - 1)) < ((rc.bytecount & 0xffffffL) >> 1))
        tmp = rc.low >> SHIFT_BITS;
    else
        tmp = (rc.low >> SHIFT_BITS) + 1;
    if (tmp > 0xff) {  // we have a carry
        output_byte(rc.buffer + 1);
        for (; rc.help; rc.help--)
            output_byte(0);
    } else {  // no carry
        output_byte(rc.buffer);
        for (; rc.help; rc.help--)
            output_byte(0xff);
    }
    output_byte(tmp & 0xff);
    output_byte((rc.bytecount >> 16) & 0xff);
    output_byte((rc.bytecount >> 8) & 0xff);
    output_byte(rc.bytecount & 0xff);
    return rc.bytecount;
}

void RangeEncoder::finish() {
    done_encoding_internal();
}

// ============================================================================
// RangeDecoder Implementation
// ============================================================================

RangeDecoder::RangeDecoder(const std::vector<uint8_t>& input)
    : input_buffer(input), position(0) {
    start_decoding_internal();
}

uint8_t RangeDecoder::input_byte() {
    if (position >= input_buffer.size()) {
        return 0;  // Return 0 if we've exhausted input
    }
    return input_buffer[position++];
}

// Schindler's start_decoding
int RangeDecoder::start_decoding_internal() {
    int c = input_byte();
    if (c == -1) {  // EOF check (won't happen with vector, but keep for compatibility)
        throw std::runtime_error("Unexpected end of input");
    }
    rc.buffer = input_byte();
    rc.low = rc.buffer >> (8 - EXTRA_BITS);
    rc.range = (code_value)1 << EXTRA_BITS;
    return c;
}

// Schindler's dec_normalize
void RangeDecoder::dec_normalize() {
    while (rc.range <= Bottom_value) {
        rc.low = (rc.low << 8) | ((rc.buffer << EXTRA_BITS) & 0xff);
        rc.buffer = input_byte();
        rc.low |= rc.buffer >> (8 - EXTRA_BITS);
        rc.range <<= 8;
    }
}

// Schindler's decode_culfreq - get cumulative frequency
freq RangeDecoder::decode_culfreq_internal(freq tot_f) {
    freq tmp;
    dec_normalize();
    rc.help = rc.range / tot_f;
    tmp = rc.low / rc.help;
    return (tmp >= tot_f ? tot_f - 1 : tmp);
}

// Public API wrapper
uint32_t RangeDecoder::get_current_count(uint32_t total_freq) {
    return decode_culfreq_internal(total_freq);
}

// Schindler's decode_update - update decoder state after symbol found
void RangeDecoder::decode_update_internal(freq sy_f, freq lt_f, freq tot_f) {
    code_value tmp;
    tmp = rc.help * lt_f;
    rc.low -= tmp;
    if (lt_f + sy_f < tot_f)
        rc.range = rc.help * sy_f;
    else
        rc.range -= tmp;
}

// Public API wrapper
void RangeDecoder::decode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq) {
    freq sy_f = cum_freq_high - cum_freq_low;
    freq lt_f = cum_freq_low;
    freq tot_f = total_freq;
    decode_update_internal(sy_f, lt_f, tot_f);
}

void RangeDecoder::done_decoding_internal() {
    dec_normalize();  // normalize to use up all bytes
}
