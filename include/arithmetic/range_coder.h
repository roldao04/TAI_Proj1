#ifndef RANGE_CODER_H
#define RANGE_CODER_H

#include <vector>
#include <cstdint>

/*
 * Range Coder Implementation
 * Based on Michael Schindler's range coder
 *
 * Academic Citation:
 * Schindler, M. (1998). "A Fast Renormalisation for Arithmetic Coding".
 * In Proceedings of the 1998 IEEE Data Compression Conference (DCC '98),
 * Snowbird, UT, IEEE Computer Society Press, p. 572.
 * DOI: 10.1109/DCC.1998.672314
 *
 * Original Implementation:
 * Copyright (c) Michael Schindler, 1997, 1998, 1999, 2000
 * http://www.compressconsult.com/rangecoder/
 *
 * License: GNU General Public License v2 or later
 *
 * Range encoding is based on an article by G.N.N. Martin, submitted
 * March 1979 and presented on the Video & Data Recording Conference,
 * Southampton, July 24-27, 1979.
 *
 * Range coding is closely related to arithmetic coding, except that
 * it does renormalisation in larger units than bits and is thus faster
 * (~2x speedup with minimal file size overhead < 0.01%).
 *
 * Adapted for TAI Project #1 by integrating with std::vector I/O
 * and C++ class-based API while preserving Schindler's core algorithm.
 */

// Type definitions to match Schindler's code
typedef uint32_t code_value;
typedef uint32_t freq;

// Schindler's rangecoder state structure
typedef struct {
    uint32_t low;           // low end of interval
    uint32_t range;         // length of interval
    uint32_t help;          // bytes_to_follow resp. intermediate value
    unsigned char buffer;   // buffer for input/output
    uint32_t bytecount;     // counter for outputed bytes
} rangecoder;

class RangeEncoder {
private:
    rangecoder rc;
    std::vector<uint8_t> output_buffer;

    // I/O helper for Schindler's code
    void output_byte(uint8_t byte);

    // Core functions from Schindler's implementation
    void start_encoding_internal(char c, int initlength);
    void enc_normalize();
    void encode_freq_internal(freq sy_f, freq lt_f, freq tot_f);
    uint32_t done_encoding_internal();

public:
    RangeEncoder();

    // API compatible with existing compressor code
    void encode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq);
    void finish();
    const std::vector<uint8_t>& get_output() const { return output_buffer; }
};

class RangeDecoder {
private:
    rangecoder rc;
    const std::vector<uint8_t>& input_buffer;
    size_t position;

    // I/O helper for Schindler's code
    uint8_t input_byte();

    // Core functions from Schindler's implementation
    int start_decoding_internal();
    void dec_normalize();
    freq decode_culfreq_internal(freq tot_f);
    void decode_update_internal(freq sy_f, freq lt_f, freq tot_f);
    void done_decoding_internal();

public:
    RangeDecoder(const std::vector<uint8_t>& input);

    // API compatible with existing decompressor code
    uint32_t get_current_count(uint32_t total_freq);
    void decode_symbol(uint32_t cum_freq_low, uint32_t cum_freq_high, uint32_t total_freq);
};

#endif // RANGE_CODER_H
