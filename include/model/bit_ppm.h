#ifndef BIT_PPM_H
#define BIT_PPM_H

#include <vector>
#include <deque>
#include <unordered_map>
#include <cstdint>
#include "model/context_mixer.h"

/**
 * Bit-level PPM context entry
 * Tracks occurrences of bit=0 and bit=1 for a given bit-context
 */
struct BitContext {
    uint32_t count0;      // Times bit=0 occurred in this context
    uint32_t count1;      // Times bit=1 occurred in this context
    uint32_t total;       // Total observations (count0 + count1)

    BitContext() : count0(0), count1(0), total(0) {}
};

/**
 * Bit-Level PPM Model
 *
 * Unlike MultiOrderPPM which uses byte contexts, this uses bit contexts
 * Context = last N bits (not last N bytes)
 * Predicts P(next_bit = 1) based on bit-history
 *
 * Example:
 *   For Order-8 with bit_history = [1,0,1,1,0,0,1,1]
 *   Context hash = hash of last 8 bits
 *   Looks up BitContext to get {count0, count1, total}
 *   Returns P(bit=1) = (count1 + 0.5) / (total + 1.0)  [Laplace smoothing]
 */
class BitLevelPPM {
private:
    std::vector<int> orders;                              // e.g., {4, 8, 12, 16, 24}
    std::unordered_map<uint64_t, BitContext> contexts;    // Hash table: context_hash → BitContext
    std::deque<bool> bit_history;                         // Recent bits (max 256 bits)
    size_t max_history_bits;                              // Maximum bit history size
    size_t hash_table_limit;                              // Memory limit (max contexts)

    /**
     * Compute hash for last N bits in history
     * Uses polynomial rolling hash
     */
    uint64_t hash_bit_context(int order);

    /**
     * Calculate confidence based on number of observations
     * 0 observations → 0.0 confidence
     * 1000+ observations → 1.0 confidence
     */
    double calculate_confidence(uint32_t total);

public:
    /**
     * Constructor
     * @param bit_orders Vector of bit orders {4, 8, 12, 16, 24}
     * @param max_contexts Maximum hash table entries (default: 16M)
     * @param max_bits Maximum bit history to keep (default: 256)
     */
    BitLevelPPM(const std::vector<int>& bit_orders,
                size_t max_contexts = 16 * 1024 * 1024,
                size_t max_bits = 256);

    /**
     * Predict probability of next bit = 1 for given order
     * @param order Bit order (4, 8, 12, etc.)
     * @return Prediction with probability P(bit=1) and confidence
     */
    Prediction predict_bit(int order);

    /**
     * Get predictions from all orders
     * @return Vector of predictions (one per order)
     */
    std::vector<Prediction> predict_all();

    /**
     * Update all contexts with actual bit value
     * Increments count0 or count1 for each order's context
     * Adds bit to history
     * @param bit Actual bit that occurred (0 or 1)
     */
    void update_all(bool bit);

    /**
     * Get memory usage in bytes
     */
    size_t get_memory_usage() const {
        return contexts.size() * sizeof(std::pair<uint64_t, BitContext>) +
               bit_history.size() * sizeof(bool);
    }

    /**
     * Get hash table utilization
     */
    size_t get_hash_table_size() const { return contexts.size(); }

    /**
     * Get maximum hash table size
     */
    size_t get_hash_table_limit() const { return hash_table_limit; }
};

#endif // BIT_PPM_H
