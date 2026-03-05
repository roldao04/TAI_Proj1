#ifndef ENTROPY_CALCULATOR_H
#define ENTROPY_CALCULATOR_H

#include <vector>
#include <cstdint>

/*
 * Entropy Calculator
 *
 * Computes Shannon entropy of data to detect compressibility.
 * Used to avoid wasting time on incompressible (random/pre-compressed) data.
 *
 * Shannon entropy: H = -Σ p(x) * log2(p(x))
 * where p(x) is the probability of symbol x
 *
 * Returns value in range [0, 8] bits/symbol:
 * - 0: Uniform (single byte repeated) - highly compressible
 * - 4-6: Natural text/structured data - compressible
 * - 7.5+: Random/encrypted/compressed - incompressible
 */

class EntropyCalculator {
public:
    // Calculate entropy of data (samples first sample_size bytes)
    // If sample_size = 0 or > data.size(), uses entire data
    static double calculate(const std::vector<uint8_t>& data, size_t sample_size = 8192);

    // Quick compressibility check
    static bool is_compressible(const std::vector<uint8_t>& data, double threshold = 7.5);

    // Classify data type based on entropy
    enum class DataType {
        HIGHLY_COMPRESSIBLE,  // entropy < 5.0 (repetitive, uniform)
        COMPRESSIBLE,         // entropy 5.0-7.0 (text, structured)
        INCOMPRESSIBLE        // entropy > 7.0 (random, encrypted, compressed)
    };

    static DataType classify(const std::vector<uint8_t>& data);
};

#endif // ENTROPY_CALCULATOR_H
