#ifndef MULTI_ORDER_PPM_H
#define MULTI_ORDER_PPM_H

#include <vector>
#include <deque>
#include <cstdint>
#include <unordered_map>
#include "model/context_mixer.h"

/**
 * Multi-Order PPM Model with Context Mixing
 *
 * Implements multiple PPM orders simultaneously:
 * - Order-1, Order-2: Direct arrays (fast, guaranteed lookup)
 * - Order-4+: Hash tables (memory efficient for sparse contexts)
 *
 * Orders used: 1, 2, 4, 6, 8, 12, 16, 24, 32
 * Each order independently predicts next bit/byte based on different context lengths
 */

// Hash table entry for high-order contexts
struct ContextEntry {
    uint64_t context_hash;      // Full context hash (for collision detection)
    uint32_t freq[258];         // Symbol frequencies (0-255 bytes + EOF + escape)
    uint32_t total;             // Sum of all frequencies
    uint32_t seen_count;        // Number of unique symbols seen
    uint32_t singleton_count;   // Number of symbols with freq==1 (for PPMD escape)
    uint16_t last_used;         // LRU timestamp (for eviction)
    uint32_t update_count;      // Number of updates (for deterministic rescaling)
    bool initialized;           // Is this entry in use?

    ContextEntry() : context_hash(0), total(0), seen_count(0),
                     singleton_count(0), last_used(0), update_count(0), initialized(false) {
        for (int i = 0; i < 258; i++) freq[i] = 0;
    }
};

class MultiOrderPPM {
private:
    // ========== Configuration ==========
    std::vector<int> enabled_orders;    // Which orders to use (e.g., {1,2,4,6,8,12,16,24,32})
    size_t hash_table_size;             // Number of hash table buckets
    size_t max_history_size;            // Maximum history buffer size
    uint16_t current_timestamp;         // For LRU tracking

    // Memory limit enforcement
    static constexpr size_t MAX_MEMORY_BYTES = 8ULL * 1024 * 1024 * 1024;  // 8GB hard limit
    static constexpr size_t MEMORY_WARNING_THRESHOLD = (MAX_MEMORY_BYTES * 80) / 100;  // Warn at 80%
    bool memory_warning_shown;          // Track if warning was already shown

    // ========== Tier 1: Direct Array (Order-1 only) ==========
    // Order-1: 256 contexts (previous byte) - small enough for direct array
    std::vector<std::vector<uint32_t>> freq1;  // [context][symbol] - 256x258
    std::vector<uint32_t> total1;              // Total frequency per context - 256
    std::vector<uint32_t> seen1;               // Unique symbols seen per context - 256
    std::vector<uint32_t> singleton1;          // Symbols with freq==1 per context - 256
    std::vector<bool> ctx_exists1;             // Has this context been seen? - 256
    std::vector<uint32_t> update_count1;       // Number of updates per context - 256 (for deterministic rescaling)

    // Order-2 now uses hash table (Tier 2) for memory efficiency
    // This saves 68 MB and makes memory adaptive to actual usage

    // ========== Tier 2: Hash Table (Order-2+) - Sparse Storage ==========
    // Using unordered_map for sparse storage - only allocates entries as needed
    // Key: context_hash, Value: ContextEntry
    std::unordered_map<uint64_t, ContextEntry> hash_table;
    size_t hash_table_capacity_limit;   // Maximum number of entries (for memory budget)

    // ========== History Buffer ==========
    std::deque<uint8_t> history;        // Recent bytes for context extraction (deque for efficient pop_front)
    size_t history_pos;                 // Current position in history

    // ========== Helper Functions ==========

    /**
     * Hash a context of given length
     */
    uint64_t hash_context(const uint8_t* data, int len);

    /**
     * Find or create hash table entry (now uses sparse unordered_map)
     * @param context_hash Hash of the context
     * @param create If true, create entry if not found; if false, return nullptr
     * @return Pointer to entry or nullptr if not found and create=false
     */
    ContextEntry* find_or_create_entry(uint64_t context_hash, bool create = true);

    /**
     * Extract context of given order from history
     * Returns pointer to context in history buffer
     */
    const uint8_t* get_context(int order);

    /**
     * Get Order-1 prediction
     */
    Prediction predict_order1(uint8_t symbol);

    /**
     * Get Order-2 prediction
     */
    Prediction predict_order2(uint8_t symbol);

    /**
     * Get high-order prediction (Order-4+) from hash table
     */
    Prediction predict_high_order(int order, uint8_t symbol);

    /**
     * Update Order-1 frequencies
     */
    void update_order1(uint8_t symbol);

    /**
     * Update Order-2 frequencies
     */
    void update_order2(uint8_t symbol);

    /**
     * Update high-order frequencies in hash table
     */
    void update_high_order(int order, uint8_t symbol);

    /**
     * Calculate confidence based on context frequency
     */
    double calculate_confidence(uint32_t total_freq, uint32_t seen_count);

    /**
     * Rescale frequencies when they get too large
     */
    void rescale_frequencies_order1(int context);
    void rescale_frequencies_order2(int context);
    void rescale_frequencies_hash(ContextEntry* entry);

public:
    /**
     * Constructor
     * @param orders Vector of orders to enable (e.g., {1,2,4,6,8,12,16,24,32})
     * @param hash_size Size of hash table in millions of contexts (default: 16M)
     * @param max_history Maximum history buffer size (default: 64KB)
     */
    MultiOrderPPM(const std::vector<int>& orders,
                  size_t hash_size = 16 * 1024 * 1024,
                  size_t max_history = 64 * 1024);

    /**
     * Get predictions from all enabled orders for a given byte
     * @param byte The byte to predict
     * @return Vector of predictions, one per enabled order
     */
    std::vector<Prediction> predict_all(uint8_t byte);

    /**
     * Get prediction for next bit (for bit-level encoding)
     * @param bit The bit to predict (0 or 1)
     * @param bit_pos Position within byte (0-7)
     * @return Vector of predictions from all orders
     */
    std::vector<Prediction> predict_bit(bool bit, int bit_pos);

    /**
     * Update all models with actual byte
     * @param byte The actual byte that occurred
     */
    void update_all(uint8_t byte);

    /**
     * Update all models with actual bit
     * @param bit The actual bit value
     * @param bit_pos Position within byte (0-7)
     */
    void update_bit(bool bit, int bit_pos);

    /**
     * Add byte to history
     */
    void add_to_history(uint8_t byte);

    /**
     * Reset all models (for new file or testing)
     */
    void reset();

    /**
     * Get frequency distribution for current context
     * These methods extract raw frequency counts from PPM models
     */

    /**
     * Get Order-1 frequency distribution
     * @param freqs Output array of 256 frequencies (caller allocates)
     * @return true if context exists, false if uniform distribution used
     */
    bool get_frequencies_order1(uint32_t* freqs);

    /**
     * Get Order-2 frequency distribution
     * @param freqs Output array of 256 frequencies (caller allocates)
     * @return true if context exists, false if uniform distribution used
     */
    bool get_frequencies_order2(uint32_t* freqs);

    /**
     * Get high-order frequency distribution (Order-4+)
     * @param order Which order to use (4, 6, 8, 12, 16, 24, 32)
     * @param freqs Output array of 256 frequencies (caller allocates)
     * @return true if context exists, false if uniform distribution used
     */
    bool get_frequencies_high_order(int order, uint32_t* freqs);

    /**
     * Get blended frequency distribution from all orders
     * Combines frequencies from all enabled orders using provided weights
     * @param freqs Output array of 256 frequencies (caller allocates)
     * @param weights_fixed Fixed-point weights for each order (must match enabled_orders size)
     */
    void get_blended_frequencies(uint32_t* freqs, const std::vector<int64_t>& weights_fixed);

    /**
     * Get statistics
     */
    size_t get_hash_table_usage() const;
    size_t get_history_size() const { return history.size(); }
    const std::vector<int>& get_enabled_orders() const { return enabled_orders; }

    /**
     * Get memory usage in bytes
     */
    size_t get_memory_usage() const;
};

#endif // MULTI_ORDER_PPM_H
