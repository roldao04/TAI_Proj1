#include "utils/entropy_calculator.h"
#include <cmath>
#include <array>
#include <algorithm>

double EntropyCalculator::calculate(const std::vector<uint8_t>& data, size_t sample_size) {
    if (data.empty()) {
        return 0.0;
    }

    // Determine actual sample size
    size_t actual_sample_size = sample_size;
    if (sample_size == 0 || sample_size > data.size()) {
        actual_sample_size = data.size();
    }

    // Count byte frequencies
    std::array<uint64_t, 256> frequencies = {0};
    for (size_t i = 0; i < actual_sample_size; i++) {
        frequencies[data[i]]++;
    }

    // Calculate Shannon entropy: H = -Σ p(x) * log2(p(x))
    double entropy = 0.0;
    for (int i = 0; i < 256; i++) {
        if (frequencies[i] > 0) {
            double probability = static_cast<double>(frequencies[i]) / actual_sample_size;
            entropy -= probability * std::log2(probability);
        }
    }

    return entropy;
}

bool EntropyCalculator::is_compressible(const std::vector<uint8_t>& data, double threshold) {
    double entropy = calculate(data);
    return entropy < threshold;
}

EntropyCalculator::DataType EntropyCalculator::classify(const std::vector<uint8_t>& data) {
    double entropy = calculate(data);

    if (entropy < 5.0) {
        return DataType::HIGHLY_COMPRESSIBLE;
    } else if (entropy < 7.0) {
        return DataType::COMPRESSIBLE;
    } else {
        return DataType::INCOMPRESSIBLE;
    }
}
