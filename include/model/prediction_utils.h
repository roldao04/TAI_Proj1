#ifndef PREDICTION_UTILS_H
#define PREDICTION_UTILS_H

#include <cmath>
#include <algorithm>

namespace PredictionUtils {

/**
 * Convert probability to log-odds (logistic/stretched domain)
 * stretch(p) = ln(p/(1-p))
 *
 * This transformation is used in PAQ-style mixing because:
 * 1. Averaging in log-odds space is more stable than averaging probabilities
 * 2. Linear combination of log-odds corresponds to logical AND/OR operations
 * 3. Avoids numerical instability near 0 and 1
 */
inline double stretch(double p) {
    // Clamp to avoid log(0) or log(negative)
    p = std::clamp(p, 0.0001, 0.9999);
    return std::log(p / (1.0 - p));
}

/**
 * Convert log-odds back to probability
 * squash(s) = 1/(1+exp(-s))
 *
 * This is the inverse of stretch(), also known as the logistic sigmoid function
 */
inline double squash(double s) {
    // Clamp to avoid overflow
    s = std::clamp(s, -10.0, 10.0);
    return 1.0 / (1.0 + std::exp(-s));
}

/**
 * Fixed-point arithmetic constants for deterministic computation
 */
constexpr int64_t FIXED_POINT_SCALE = 65536;  // 2^16 (16-bit fractional precision)
constexpr int PROB_SCALE = 4096;              // Probability scale (12-bit)
constexpr int STRETCH_SCALE = 2048;           // Stretched value scale (±10.0 range)

/**
 * Fast integer stretch using lookup table (for performance)
 * Maps probability [0, 4095] to stretched value
 */
class FastStretch {
private:
    static constexpr int TABLE_SIZE = 4096;
    double stretch_table[TABLE_SIZE];
    double squash_table[TABLE_SIZE];

    // Fixed-point lookup tables (deterministic integer arithmetic)
    int32_t stretch_table_fixed[TABLE_SIZE];  // Maps prob [0,4095] -> stretched*2048
    int32_t squash_table_fixed[TABLE_SIZE];   // Maps stretched*2048 -> prob [0,4095]

public:
    FastStretch() {
        // Build floating-point lookup tables
        for (int i = 0; i < TABLE_SIZE; i++) {
            double p = (i + 0.5) / TABLE_SIZE;
            stretch_table[i] = stretch(p);

            double s = (i - TABLE_SIZE/2) * 0.01;  // Range: -20.48 to 20.47
            squash_table[i] = squash(s);
        }

        // Build fixed-point lookup tables for deterministic operations
        for (int i = 0; i < TABLE_SIZE; i++) {
            // Stretch: probability [0,4095] -> stretched value
            double p = (i + 0.5) / TABLE_SIZE;
            double stretched = stretch(p);
            stretch_table_fixed[i] = (int32_t)(stretched * STRETCH_SCALE);

            // Squash: stretched value -> probability [0,4095]
            // Map index to stretched range [-10.0, +10.0]
            double s = ((i - TABLE_SIZE/2.0) / TABLE_SIZE) * 20.0;
            double prob = squash(s);
            squash_table_fixed[i] = (int32_t)(prob * PROB_SCALE);
        }
    }

    double fast_stretch(double p) const {
        int index = std::clamp((int)(p * TABLE_SIZE), 0, TABLE_SIZE - 1);
        return stretch_table[index];
    }

    double fast_squash(double s) const {
        // Map stretched value to table index
        int index = std::clamp((int)((s + 20.48) * 100), 0, TABLE_SIZE - 1);
        return squash_table[index];
    }

    /**
     * Fixed-point stretch: converts probability to stretched (log-odds)
     * @param prob Probability in range [0, PROB_SCALE] (4096)
     * @return Stretched value multiplied by STRETCH_SCALE (2048)
     */
    int32_t stretch_fixed(int32_t prob) const {
        int index = std::clamp((int)prob, 0, TABLE_SIZE - 1);
        return stretch_table_fixed[index];
    }

    /**
     * Fixed-point squash: converts stretched (log-odds) to probability
     * @param stretched Stretched value multiplied by STRETCH_SCALE (2048)
     * @return Probability in range [0, PROB_SCALE] (4096)
     */
    int32_t squash_fixed(int32_t stretched) const {
        // Map stretched [-10*2048, +10*2048] to index [0, 4095]
        // stretched range: [-20480, +20480]
        // Map to [0, 4095]: (stretched + 20480) * 4096 / 40960
        int index = (int)((stretched + 20480) * TABLE_SIZE / 40960);
        index = std::clamp(index, 0, TABLE_SIZE - 1);
        return squash_table_fixed[index];
    }
};

// Global fast stretch instance (initialized once)
extern FastStretch global_fast_stretch;

/**
 * Helper functions for fixed-point conversions
 */
inline int64_t double_to_fixed(double value, int64_t scale = FIXED_POINT_SCALE) {
    return (int64_t)(value * scale);
}

inline double fixed_to_double(int64_t value, int64_t scale = FIXED_POINT_SCALE) {
    return (double)value / scale;
}

} // namespace PredictionUtils

#endif // PREDICTION_UTILS_H
