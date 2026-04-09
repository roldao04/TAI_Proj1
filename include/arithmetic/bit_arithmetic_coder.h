#ifndef BIT_ARITHMETIC_CODER_H
#define BIT_ARITHMETIC_CODER_H

#include <vector>
#include <cstdint>
#include <stdexcept>

/**
 * Bit-level arithmetic coder for PAQ-style compression
 *
 * Unlike the byte-level range coder, this works with individual bits
 * and probabilities P(0) vs P(1), which is required for context mixing
 * where we predict bits rather than bytes.
 */

class BitArithmeticEncoder {
private:
    uint64_t low;                    // Lower bound of current range (64-bit for better precision)
    uint64_t high;                   // Upper bound of current range (64-bit for better precision)
    std::vector<uint8_t>& output;    // Output buffer
    uint64_t pending_bits;           // Bits waiting to be output (for carry handling)

    void output_bit(bool bit);
    void renormalize();

public:
    explicit BitArithmeticEncoder(std::vector<uint8_t>& out);

    /**
     * Encode a single bit given probability that bit is 0
     * @param bit The bit to encode (0 or 1)
     * @param p_zero Probability that bit is 0 (range: 0.0 to 1.0)
     */
    void encode_bit(bool bit, double p_zero);

    /**
     * Finish encoding and flush remaining bits
     */
    void finish();

    /**
     * Get current number of bytes output
     */
    size_t get_output_size() const { return output.size(); }
};

class BitArithmeticDecoder {
private:
    uint64_t low;                          // Lower bound of current range (64-bit for better precision)
    uint64_t high;                         // Upper bound of current range (64-bit for better precision)
    uint64_t value;                        // Current value within range (64-bit for better precision)
    const std::vector<uint8_t>& input;     // Input buffer
    size_t input_pos;                      // Current position in input

    void renormalize();
    uint8_t read_byte();

public:
    explicit BitArithmeticDecoder(const std::vector<uint8_t>& in);

    /**
     * Decode a single bit given probability that bit is 0
     * @param p_zero Probability that bit is 0 (range: 0.0 to 1.0)
     * @return The decoded bit (0 or 1)
     */
    bool decode_bit(double p_zero);

    /**
     * Check if more input is available
     */
    bool has_more_input() const { return input_pos < input.size(); }

    /**
     * Get current input position
     */
    size_t get_position() const { return input_pos; }
};

#endif // BIT_ARITHMETIC_CODER_H
