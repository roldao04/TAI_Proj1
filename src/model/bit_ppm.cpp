#include "model/bit_ppm.h"
#include <algorithm>
#include <cmath>
#include <iostream>

// ============================================================================
// BitLevelPPM Implementation
// ============================================================================

BitLevelPPM::BitLevelPPM(const std::vector<int>& bit_orders,
                         size_t max_contexts,
                         size_t max_bits)
    : orders(bit_orders),
      max_history_bits(max_bits),
      hash_table_limit(max_contexts) {

    // Reserve space for hash table to reduce reallocations
    contexts.reserve(std::min(max_contexts, (size_t)1024 * 1024));  // Reserve up to 1M initially

    // Validate orders
    for (int order : orders) {
        if (order < 1 || order > 64) {
            throw std::invalid_argument("Bit order must be between 1 and 64");
        }
        if (order > (int)max_history_bits) {
            throw std::invalid_argument("Bit order cannot exceed max_history_bits");
        }
    }

    std::cout << "BitLevelPPM initialized with orders: ";
    for (int order : orders) std::cout << order << " ";
    std::cout << "\n";
    std::cout << "Hash table limit: " << (hash_table_limit / 1024) << " K contexts\n";
    std::cout << "Max history: " << max_history_bits << " bits\n";
}

uint64_t BitLevelPPM::hash_bit_context(int order) {
    if (bit_history.size() < (size_t)order) {
        return 0;  // Not enough history
    }

    // Polynomial rolling hash of last 'order' bits
    // For small orders (<= 32), this is just the bit pattern itself
    uint64_t hash = 0;
    size_t start_pos = bit_history.size() - order;

    for (size_t i = start_pos; i < bit_history.size(); i++) {
        hash = (hash << 1) | (bit_history[i] ? 1ULL : 0ULL);
    }

    return hash;
}

double BitLevelPPM::calculate_confidence(uint32_t total) {
    // Confidence increases with more observations
    // 0 observations → 0.0 confidence
    // 100+ observations → 1.0 confidence
    // Balanced threshold - not too aggressive, not too conservative
    return std::min(1.0, total / 100.0);
}

Prediction BitLevelPPM::predict_bit(int order) {
    if (bit_history.size() < (size_t)order) {
        // Not enough history - return neutral prediction
        return Prediction(0.5, 0.0, false, order, 0);
    }

    uint64_t ctx_hash = hash_bit_context(order);
    auto it = contexts.find(ctx_hash);

    if (it == contexts.end() || it->second.total == 0) {
        // Context never seen - return neutral with low confidence
        return Prediction(0.5, 0.0, false, order, 0);
    }

    const BitContext& ctx = it->second;

    // Laplace smoothing: P(bit=1) = (count1 + 0.5) / (total + 1.0)
    // This prevents P=0 or P=1 which cause problems for arithmetic coder
    double p_bit1 = (ctx.count1 + 0.5) / (ctx.total + 1.0);

    // Clamp to safe range for arithmetic coder
    // Matches the clamping in bit_arithmetic_coder.cpp
    p_bit1 = std::clamp(p_bit1, 0.0005, 0.9995);

    double confidence = calculate_confidence(ctx.total);

    return Prediction(p_bit1, confidence, true, order, ctx.total);
}

std::vector<Prediction> BitLevelPPM::predict_all() {
    std::vector<Prediction> predictions;
    predictions.reserve(orders.size());

    for (int order : orders) {
        predictions.push_back(predict_bit(order));
    }

    return predictions;
}

void BitLevelPPM::update_all(bool bit) {
    // Update all order contexts
    for (int order : orders) {
        if (bit_history.size() >= (size_t)order) {
            uint64_t ctx_hash = hash_bit_context(order);

            // Check if we're at memory limit
            if (contexts.size() >= hash_table_limit && contexts.find(ctx_hash) == contexts.end()) {
                // Hash table full, skip creating new context
                // This gracefully degrades performance instead of crashing
                continue;
            }

            // Create or update context
            BitContext& ctx = contexts[ctx_hash];

            if (bit) {
                ctx.count1++;
            } else {
                ctx.count0++;
            }
            ctx.total++;

            // Rescale if needed to prevent overflow
            // When counts get too large, halve them (with rounding)
            if (ctx.total > 65535) {
                ctx.count0 = (ctx.count0 + 1) / 2;
                ctx.count1 = (ctx.count1 + 1) / 2;
                ctx.total = ctx.count0 + ctx.count1;
            }
        }
    }

    // Add bit to history
    bit_history.push_back(bit);

    // Remove oldest bit if history is full
    if (bit_history.size() > max_history_bits) {
        bit_history.pop_front();
    }
}
