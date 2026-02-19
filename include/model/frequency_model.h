#ifndef FREQUENCY_MODEL_H
#define FREQUENCY_MODEL_H

#include <vector>
#include <cstdint>
#include <array>

/*
 * Order-0 Frequency Model
 *
 * This model maintains frequency counts for each byte value (0-255)
 * plus an EOF symbol, and provides cumulative frequency information
 * required by the arithmetic coder.
 *
 * The model supports two modes:
 * 1. Static: frequencies are counted in a first pass, then used for encoding
 * 2. Adaptive: frequencies can be updated during encoding/decoding
 */

class FrequencyModel {
private:
    static const int ALPHABET_SIZE = 257;  // 256 bytes + 1 EOF symbol
    static const int EOF_SYMBOL = 256;

    std::array<uint32_t, ALPHABET_SIZE> frequencies;
    std::array<uint32_t, ALPHABET_SIZE> cumulative_freq;
    uint32_t total_freq;

    void build_cumulative_freq();

public:
    FrequencyModel();

    // Build model from data
    void build_from_data(const std::vector<uint8_t>& data);

    // Get cumulative frequency range for a symbol
    void get_symbol_range(int symbol, uint32_t& cum_freq_low, uint32_t& cum_freq_high) const;

    // Find symbol from cumulative frequency
    int find_symbol(uint32_t cum_freq) const;

    // Get total frequency
    uint32_t get_total_freq() const { return total_freq; }

    // Get frequencies (for saving model to compressed file)
    const std::array<uint32_t, ALPHABET_SIZE>& get_frequencies() const { return frequencies; }

    // Set frequencies (for loading model from compressed file)
    void set_frequencies(const std::array<uint32_t, ALPHABET_SIZE>& freq);

    // Get EOF symbol
    static int get_eof_symbol() { return EOF_SYMBOL; }
};

#endif // FREQUENCY_MODEL_H
