#include "model/multi_order_ppm.h"
#include <cstring>
#include <algorithm>
#include <iostream>
#include <cmath>

// Simple fast hash function (xxHash-like)
static inline uint64_t xxhash64_simple(const uint8_t* data, int len, uint64_t seed = 0) {
    const uint64_t PRIME1 = 11400714785074694791ULL;
    const uint64_t PRIME2 = 14029467366897019727ULL;
    const uint64_t PRIME3 = 1609587929392839161ULL;
    const uint64_t PRIME5 = 2870177450012600261ULL;

    uint64_t hash = seed + PRIME5;

    for (int i = 0; i < len; i++) {
        hash ^= data[i] * PRIME5;
        hash = ((hash << 11) | (hash >> 53)) * PRIME1;
    }

    hash ^= hash >> 33;
    hash *= PRIME2;
    hash ^= hash >> 29;
    hash *= PRIME3;
    hash ^= hash >> 32;

    return hash;
}

// ============================================================================
// Constructor and Initialization
// ============================================================================

MultiOrderPPM::MultiOrderPPM(const std::vector<int>& orders,
                             size_t hash_capacity_limit,
                             size_t max_history)
    : enabled_orders(orders),
      max_history_size(max_history),
      hash_table_capacity_limit(hash_capacity_limit),
      current_timestamp(0),
      history_pos(0),
      memory_warning_shown(false) {

    // Sort orders for consistent processing
    std::sort(enabled_orders.begin(), enabled_orders.end());

    // Initialize Order-1 vectors (256 contexts - small, use direct array)
    freq1.resize(256, std::vector<uint32_t>(258, 0));
    total1.resize(256, 0);
    seen1.resize(256, 0);
    singleton1.resize(256, 0);
    ctx_exists1.resize(256, false);
    update_count1.resize(256, 0);  // Initialize update counts for deterministic rescaling

    // Order-2 and higher use sparse hash table (no pre-allocation!)
    // Using unordered_map - only allocates entries as needed
    // Reserve MORE buckets upfront to avoid expensive rehashing during compression
    // This prevents bad_alloc during rehash operations
    hash_table.reserve(std::min(hash_capacity_limit, (size_t)65536));

    // CRITICAL: Set high max_load_factor to prevent rehashing
    // Default load factor of 1.0 triggers expensive rehashing when size() > bucket_count()
    // Rehashing can cause bad_alloc due to memory fragmentation
    // A higher load factor allows more entries per bucket before triggering rehash
    hash_table.max_load_factor(10.0);

    // Note: deque doesn't need reserve(), it grows efficiently

    std::cout << "MultiOrderPPM initialized with orders: ";
    for (int order : enabled_orders) {
        std::cout << order << " ";
    }
    std::cout << "\nHash table capacity limit: " << (hash_capacity_limit / 1024) << " K contexts" << std::endl;
    std::cout << "Initial memory usage: ~" << (get_memory_usage() / (1024*1024)) << " MB (sparse, grows as needed)" << std::endl;
}

// ============================================================================
// Hash and Lookup Functions
// ============================================================================

uint64_t MultiOrderPPM::hash_context(const uint8_t* data, int len) {
    return xxhash64_simple(data, len);
}

ContextEntry* MultiOrderPPM::find_or_create_entry(uint64_t context_hash, bool create) {
    // Sparse hash table using unordered_map - much simpler!

    auto it = hash_table.find(context_hash);

    if (it != hash_table.end()) {
        // Found existing entry
        it->second.last_used = current_timestamp++;
        return &it->second;
    }

    // Entry not found
    if (!create) {
        return nullptr;
    }

    // ========== MEMORY LIMIT ENFORCEMENT ==========
    // Check memory usage before creating new context
    size_t current_memory = get_memory_usage();

    // Hard limit: Refuse to allocate if we'd exceed 8GB
    if (current_memory + sizeof(ContextEntry) > MAX_MEMORY_BYTES) {
        static bool limit_warned = false;
        if (!limit_warned) {
            std::cerr << "\n⚠️  MEMORY LIMIT REACHED: " << (current_memory / (1024*1024))
                      << " MB / " << (MAX_MEMORY_BYTES / (1024*1024)) << " MB" << std::endl;
            std::cerr << "    Refusing to create new contexts. Compression will continue with existing contexts." << std::endl;
            limit_warned = true;
        }
        return nullptr;  // Refuse allocation
    }

    // Warning at 80% memory usage
    if (!memory_warning_shown && current_memory > MEMORY_WARNING_THRESHOLD) {
        std::cerr << "\n⚠️  Memory usage at " << (current_memory * 100 / MAX_MEMORY_BYTES)
                  << "% (" << (current_memory / (1024*1024)) << " MB / "
                  << (MAX_MEMORY_BYTES / (1024*1024)) << " MB)" << std::endl;
        memory_warning_shown = true;
    }
    // ===============================================

    // Check if we've hit hash table capacity limit
    if (hash_table.size() >= hash_table_capacity_limit) {
        // Allow growing beyond capacity limit (within memory budget)
        // LRU eviction is too expensive (O(n) scan)
        static bool capacity_warned = false;
        if (!capacity_warned) {
            std::cerr << "Info: Hash table size (" << hash_table.size()
                      << ") exceeded capacity limit (" << hash_table_capacity_limit
                      << "), allowing growth (within 8GB memory limit)" << std::endl;
            capacity_warned = true;
        }
    }

    // Create new entry
    ContextEntry new_entry;
    new_entry.context_hash = context_hash;
    new_entry.initialized = true;
    new_entry.last_used = current_timestamp++;

    // Initialize frequencies to 0 (will be properly initialized on first update)
    for (int i = 0; i < 258; i++) {
        new_entry.freq[i] = 0;
    }
    new_entry.total = 0;
    new_entry.seen_count = 0;
    new_entry.singleton_count = 0;

    auto result = hash_table.emplace(context_hash, new_entry);
    return &result.first->second;
}

const uint8_t* MultiOrderPPM::get_context(int order) {
    if (history.size() < (size_t)order) {
        return nullptr;
    }
    return &history[history.size() - order];
}

// ============================================================================
// Prediction Functions
// ============================================================================

Prediction MultiOrderPPM::predict_order1(uint8_t symbol) {
    if (history.empty()) {
        return Prediction(0.5, 0.0, false, 1, 0);
    }

    uint8_t ctx = history.back();

    if (!ctx_exists1[ctx]) {
        return Prediction(0.5, 0.0, false, 1, 0);
    }

    uint32_t total = total1[ctx];
    uint32_t freq = freq1[ctx][symbol];

    if (freq == 0) {
        // Symbol not seen in this context
        // Use escape probability
        uint32_t escape_freq = std::max(1U, singleton1[ctx]);
        double prob = (double)escape_freq / (total + escape_freq);
        double conf = calculate_confidence(total, seen1[ctx]);
        return Prediction(prob, conf * 0.5, true, 1, total);
    }

    // Symbol seen, use its frequency
    double prob = (double)freq / total;
    double conf = calculate_confidence(total, seen1[ctx]);
    return Prediction(prob, conf, true, 1, total);
}

Prediction MultiOrderPPM::predict_order2(uint8_t symbol) {
    if (history.size() < 2) {
        return Prediction(0.5, 0.0, false, 2, 0);
    }

    // Order-2 now uses hash table (like higher orders)
    const uint8_t* ctx_data = get_context(2);
    if (!ctx_data) {
        return Prediction(0.5, 0.0, false, 2, 0);
    }

    uint64_t ctx_hash = hash_context(ctx_data, 2);
    ContextEntry* entry = find_or_create_entry(ctx_hash, false);

    if (!entry || !entry->initialized) {
        return Prediction(0.5, 0.0, false, 2, 0);
    }

    uint32_t total = entry->total;
    uint32_t freq = entry->freq[symbol];

    if (freq == 0) {
        uint32_t escape_freq = std::max(1U, entry->singleton_count);
        double prob = (double)escape_freq / (total + escape_freq);
        double conf = calculate_confidence(total, entry->seen_count);
        return Prediction(prob, conf * 0.5, true, 2, total);
    }

    double prob = (double)freq / total;
    double conf = calculate_confidence(total, entry->seen_count);
    return Prediction(prob, conf, true, 2, total);
}

Prediction MultiOrderPPM::predict_high_order(int order, uint8_t symbol) {
    const uint8_t* ctx_data = get_context(order);
    if (!ctx_data) {
        return Prediction(0.5, 0.0, false, order, 0);
    }

    uint64_t ctx_hash = hash_context(ctx_data, order);
    ContextEntry* entry = find_or_create_entry(ctx_hash, false);

    if (!entry || !entry->initialized) {
        return Prediction(0.5, 0.0, false, order, 0);
    }

    uint32_t total = entry->total;
    uint32_t freq = entry->freq[symbol];

    if (freq == 0) {
        uint32_t escape_freq = std::max(1U, entry->singleton_count);
        double prob = (double)escape_freq / (total + escape_freq);
        double conf = calculate_confidence(total, entry->seen_count);
        return Prediction(prob, conf * 0.5, true, order, total);
    }

    double prob = (double)freq / total;
    double conf = calculate_confidence(total, entry->seen_count);
    return Prediction(prob, conf, true, order, total);
}

std::vector<Prediction> MultiOrderPPM::predict_all(uint8_t byte) {
    std::vector<Prediction> predictions;
    predictions.reserve(enabled_orders.size());

    for (int order : enabled_orders) {
        Prediction pred;
        if (order == 1) {
            pred = predict_order1(byte);
        } else if (order == 2) {
            pred = predict_order2(byte);
        } else {
            pred = predict_high_order(order, byte);
        }
        predictions.push_back(pred);
    }

    return predictions;
}

// ============================================================================
// Update Functions
// ============================================================================

void MultiOrderPPM::update_order1(uint8_t symbol) {
    if (history.empty()) return;

    uint8_t ctx = history[history.size() - 1];

    if (!ctx_exists1[ctx]) {
        // First time seeing this context
        ctx_exists1[ctx] = true;
        // Initialize with uniform probabilities
        for (int i = 0; i < 256; i++) {
            freq1[ctx][i] = 1;
        }
        freq1[ctx][256] = 1;  // EOF
        freq1[ctx][257] = 1;  // Escape
        total1[ctx] = 258;
        seen1[ctx] = 0;
        singleton1[ctx] = 258;  // All singletons initially
    }

    // Update frequencies
    uint32_t old_freq = freq1[ctx][symbol];
    freq1[ctx][symbol]++;
    total1[ctx]++;

    // Update singleton count
    if (old_freq == 0) {
        seen1[ctx]++;
        singleton1[ctx]++;
    } else if (old_freq == 1) {
        singleton1[ctx]--;
    }

    // DETERMINISTIC RESCALING: Rescale after fixed number of updates (256)
    // This ensures encoder and decoder rescale at the same time
    update_count1[ctx]++;
    if (update_count1[ctx] % 256 == 0) {
        rescale_frequencies_order1(ctx);
        update_count1[ctx] = 0;  // Reset counter after rescaling
    }
}

void MultiOrderPPM::update_order2(uint8_t symbol) {
    if (history.size() < 2) return;

    // Order-2 now uses hash table (like higher orders)
    const uint8_t* ctx_data = get_context(2);
    if (!ctx_data) return;

    uint64_t ctx_hash = hash_context(ctx_data, 2);
    ContextEntry* entry = find_or_create_entry(ctx_hash, true);

    if (!entry) return;  // Shouldn't happen

    if (!entry->initialized || entry->total == 0) {
        // Initialize new context
        for (int i = 0; i < 258; i++) {
            entry->freq[i] = 1;
        }
        entry->total = 258;
        entry->seen_count = 0;
        entry->singleton_count = 258;
    }

    uint32_t old_freq = entry->freq[symbol];
    entry->freq[symbol]++;
    entry->total++;

    if (old_freq == 0) {
        entry->seen_count++;
        entry->singleton_count++;
    } else if (old_freq == 1) {
        entry->singleton_count--;
    }

    // DETERMINISTIC RESCALING: Rescale after fixed number of updates (256)
    entry->update_count++;
    if (entry->update_count % 256 == 0) {
        rescale_frequencies_hash(entry);
        entry->update_count = 0;  // Reset counter after rescaling
    }
}

void MultiOrderPPM::update_high_order(int order, uint8_t symbol) {
    const uint8_t* ctx_data = get_context(order);
    if (!ctx_data) return;

    uint64_t ctx_hash = hash_context(ctx_data, order);
    ContextEntry* entry = find_or_create_entry(ctx_hash, true);

    if (!entry) return;  // Shouldn't happen

    if (!entry->initialized || entry->total == 0) {
        // Initialize new context
        for (int i = 0; i < 258; i++) {
            entry->freq[i] = 1;
        }
        entry->total = 258;
        entry->seen_count = 0;
        entry->singleton_count = 258;
    }

    uint32_t old_freq = entry->freq[symbol];
    entry->freq[symbol]++;
    entry->total++;

    if (old_freq == 0) {
        entry->seen_count++;
        entry->singleton_count++;
    } else if (old_freq == 1) {
        entry->singleton_count--;
    }

    // Deterministic rescaling: rescale every 256 updates (not threshold-based)
    // This ensures encoder and decoder rescale at EXACTLY the same time
    entry->update_count++;
    if (entry->update_count % 256 == 0) {
        rescale_frequencies_hash(entry);
        entry->update_count = 0;  // Reset counter after rescaling
    }
}

void MultiOrderPPM::update_all(uint8_t byte) {
    for (int order : enabled_orders) {
        if (order == 1) {
            update_order1(byte);
        } else if (order == 2) {
            update_order2(byte);
        } else {
            update_high_order(order, byte);
        }
    }

    add_to_history(byte);
}

// ============================================================================
// Helper Functions
// ============================================================================

void MultiOrderPPM::add_to_history(uint8_t byte) {
    history.push_back(byte);
    if (history.size() > max_history_size) {
        // Remove oldest byte (deque makes this O(1))
        history.pop_front();
    }
    history_pos++;
}

double MultiOrderPPM::calculate_confidence(uint32_t total_freq, uint32_t seen_count) {
    // Confidence increases with more observations
    // Cap at 1.0 when we have 1000+ observations
    double freq_conf = std::min(1.0, total_freq / 1000.0);

    // Also factor in diversity (more diverse = slightly less confident)
    double diversity_factor = seen_count > 0 ? (1.0 - (seen_count / 256.0) * 0.2) : 1.0;

    return freq_conf * diversity_factor;
}

void MultiOrderPPM::rescale_frequencies_order1(int context) {
    uint32_t new_total = 0;
    uint32_t new_singleton = 0;

    for (int i = 0; i < 258; i++) {
        freq1[context][i] = (freq1[context][i] + 1) / 2;  // Halve, round up
        new_total += freq1[context][i];
        if (freq1[context][i] == 1) {
            new_singleton++;
        }
    }

    total1[context] = new_total;
    singleton1[context] = new_singleton;
}

// Order-2 now uses hash table, so rescale_frequencies_order2 is removed
// It uses rescale_frequencies_hash instead

void MultiOrderPPM::rescale_frequencies_hash(ContextEntry* entry) {
    uint32_t new_total = 0;
    uint32_t new_singleton = 0;

    for (int i = 0; i < 258; i++) {
        entry->freq[i] = (entry->freq[i] + 1) / 2;
        new_total += entry->freq[i];
        if (entry->freq[i] == 1) {
            new_singleton++;
        }
    }

    entry->total = new_total;
    entry->singleton_count = new_singleton;
}

void MultiOrderPPM::reset() {
    // Reset Order-1
    for (int i = 0; i < 256; i++) {
        std::fill(freq1[i].begin(), freq1[i].end(), 0);
        total1[i] = 0;
        seen1[i] = 0;
        singleton1[i] = 0;
        ctx_exists1[i] = false;
    }

    // Order-2 and higher use hash table - just clear it
    hash_table.clear();

    // Clear history
    history.clear();
    history_pos = 0;
    current_timestamp = 0;
}

size_t MultiOrderPPM::get_hash_table_usage() const {
    return hash_table.size();
}

size_t MultiOrderPPM::get_memory_usage() const {
    size_t total = 0;

    // Order-1 vectors: 256 contexts × 258 symbols × 4 bytes
    total += 256 * 258 * sizeof(uint32_t);
    total += 256 * sizeof(uint32_t) * 3;  // total1, seen1, singleton1
    total += 256 * sizeof(bool);          // ctx_exists1

    // Sparse hash table (only counts actually used entries)
    // Each entry: ~1KB, only allocated as needed
    total += hash_table.size() * sizeof(ContextEntry);

    // unordered_map overhead (bucket array)
    total += hash_table.bucket_count() * sizeof(void*);

    // History buffer (deque: approximate as size * element size)
    total += history.size() * sizeof(uint8_t);

    return total;
}

// ============================================================================
// Frequency Distribution Extraction
// ============================================================================

bool MultiOrderPPM::get_frequencies_order1(uint32_t* freqs) {
    if (history.empty()) {
        // No context - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    uint8_t ctx = history.back();

    if (!ctx_exists1[ctx]) {
        // Context not seen - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    // Copy frequencies from this context
    for (int i = 0; i < 256; i++) {
        freqs[i] = freq1[ctx][i];
        if (freqs[i] == 0) freqs[i] = 1;  // Ensure non-zero (escape)
    }

    return true;
}

bool MultiOrderPPM::get_frequencies_order2(uint32_t* freqs) {
    if (history.size() < 2) {
        // Not enough context - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    // Order-2 now uses hash table (like higher orders)
    const uint8_t* ctx_data = get_context(2);
    if (!ctx_data) {
        // Not enough history - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    uint64_t ctx_hash = hash_context(ctx_data, 2);
    ContextEntry* entry = find_or_create_entry(ctx_hash, false);

    if (!entry || !entry->initialized) {
        // Context not seen - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    // Copy frequencies from this context
    for (int i = 0; i < 256; i++) {
        freqs[i] = entry->freq[i];
        if (freqs[i] == 0) freqs[i] = 1;  // Ensure non-zero (escape)
    }

    return true;
}

bool MultiOrderPPM::get_frequencies_high_order(int order, uint32_t* freqs) {
    const uint8_t* ctx_data = get_context(order);
    if (!ctx_data) {
        // Not enough history - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    uint64_t ctx_hash = hash_context(ctx_data, order);
    ContextEntry* entry = find_or_create_entry(ctx_hash, false);

    if (!entry || !entry->initialized) {
        // Context not seen - return uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return false;
    }

    // Copy frequencies from this context
    for (int i = 0; i < 256; i++) {
        freqs[i] = entry->freq[i];
        if (freqs[i] == 0) freqs[i] = 1;  // Ensure non-zero (escape)
    }

    return true;
}

void MultiOrderPPM::get_blended_frequencies(uint32_t* freqs, const std::vector<int64_t>& weights_fixed) {
    static bool debug_once = false;  // Only print debug for first few calls
    static int debug_count = 0;
    // Disable debug output for production
    bool do_debug = false;

    if (weights_fixed.size() != enabled_orders.size()) {
        std::cerr << "Warning: weights size (" << weights_fixed.size()
                  << ") != enabled_orders size (" << enabled_orders.size() << ")" << std::endl;
        // Fallback to uniform
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
        return;
    }

    // ========== DETERMINISTIC INTEGER ARITHMETIC (NO FLOATING POINT) ==========
    // Use fixed-point arithmetic to ensure encoder/decoder produce IDENTICAL results
    // weights_fixed are already in fixed-point format (scaled by FIXED_POINT_SCALE = 65536)

    const uint32_t OUTPUT_SCALE = 10000;  // Output frequency scale

    if (do_debug) {
        std::cerr << "\n[DEBUG get_blended_frequencies call #" << debug_count << "]\n";
        std::cerr << "  OUTPUT_SCALE = " << OUTPUT_SCALE << "\n";
        std::cerr << "  num_orders = " << enabled_orders.size() << "\n";
    }

    // Use the provided fixed-point weights directly (no conversion needed!)
    uint64_t int_weights[16];  // Max 16 orders should be enough
    uint64_t total_int_weight = 0;

    size_t num_orders = std::min(weights_fixed.size(), enabled_orders.size());
    for (size_t i = 0; i < num_orders; i++) {
        // Use weights directly from mixer (already in fixed-point format)
        // Clamp negative weights to 0 (should never happen, but be safe)
        int_weights[i] = std::max((int64_t)0, weights_fixed[i]);
        total_int_weight += int_weights[i];

        if (do_debug) {
            std::cerr << "  weight[" << i << "] = " << int_weights[i]
                      << " (" << (int_weights[i] / 65536.0) << " float)\n";
        }
    }

    // Sanity check - if all weights are zero, use equal weighting
    if (total_int_weight == 0) {
        if (do_debug) std::cerr << "  WARNING: All weights zero, using equal weighting\n";
        for (size_t i = 0; i < num_orders; i++) {
            int_weights[i] = PredictionUtils::FIXED_POINT_SCALE;  // 1.0 in fixed-point
            total_int_weight += PredictionUtils::FIXED_POINT_SCALE;
        }
    }

    // Initialize blended frequencies to zero (using integers!)
    uint64_t blended[256];
    for (int i = 0; i < 256; i++) {
        blended[i] = 0;
    }

    uint64_t total_effective_weight = 0;

    // Blend frequencies from all orders using pure integer arithmetic
    for (size_t order_idx = 0; order_idx < num_orders; order_idx++) {
        int order = enabled_orders[order_idx];
        uint64_t weight = int_weights[order_idx];

        if (weight == 0) continue;  // Skip zero-weight orders

        uint32_t order_freqs[256];
        bool context_exists = false;

        // Get frequencies for this order
        if (order == 1) {
            context_exists = get_frequencies_order1(order_freqs);
        } else if (order == 2) {
            context_exists = get_frequencies_order2(order_freqs);
        } else {
            context_exists = get_frequencies_high_order(order, order_freqs);
        }

        if (do_debug) {
            uint32_t order_sum = 0;
            for (int i = 0; i < 256; i++) order_sum += order_freqs[i];
            std::cerr << "  Order-" << order << ": context_exists=" << context_exists
                      << ", sum(order_freqs)=" << order_sum
                      << ", first 3 freqs: [" << order_freqs[0] << "," << order_freqs[1] << "," << order_freqs[2] << "]\n";
        }

        // Add weighted frequencies (all integer operations!)
        // PRECISION FIX: Apply weight reduction AFTER multiplication to preserve precision
        // Old approach: effective_weight = weight / 10, then (effective_weight * freq) / SCALE
        //   Problem: (65536/10 * 1) / 65536 = 6553 / 65536 = 0 (rounds down!)
        // New approach: (weight * freq) / SCALE, then divide result by 10 if needed
        //   Better: (65536 * 1) / 65536 / 10 = 1 / 10 = 0 (still rounds down, but later)
        // Best approach: (weight * freq * 10) / (SCALE * 10) if !context_exists
        for (int i = 0; i < 256; i++) {
            uint64_t contribution;
            if (context_exists) {
                // Full weight contribution
                contribution = (weight * order_freqs[i]) / PredictionUtils::FIXED_POINT_SCALE;
            } else {
                // Reduced weight (10x less): multiply by 1, divide by 10 extra
                // To maintain precision: (weight * freq) / (SCALE * 10)
                contribution = (weight * order_freqs[i]) / (PredictionUtils::FIXED_POINT_SCALE * 10);
            }
            blended[i] += contribution;
        }

        // Track total effective weight (for debugging)
        uint64_t effective_weight = context_exists ? weight : (weight / 10);
        total_effective_weight += effective_weight;
    }

    // Normalize so the SUM of all frequencies equals OUTPUT_SCALE
    // First, compute the sum of all blended frequencies
    uint64_t total_blended = 0;
    for (int i = 0; i < 256; i++) {
        total_blended += blended[i];
    }

    if (do_debug) {
        std::cerr << "  total_blended = " << total_blended << "\n";
        std::cerr << "  First 5 blended values: [";
        for (int i = 0; i < 5; i++) {
            std::cerr << blended[i];
            if (i < 4) std::cerr << ",";
        }
        std::cerr << "]\n";
    }

    // Now normalize each frequency: freq[i] = blended[i] * OUTPUT_SCALE / total_blended
    if (total_blended > 0) {
        for (int i = 0; i < 256; i++) {
            // Integer division with rounding: (a * b + c/2) / c
            // This ensures identical results on encoder/decoder
            uint64_t normalized = (blended[i] * OUTPUT_SCALE + total_blended / 2) / total_blended;
            // Ensure minimum frequency of 1 (required by range coder)
            freqs[i] = (uint32_t)std::max((uint64_t)1, normalized);
        }
    } else {
        if (do_debug) std::cerr << "  WARNING: total_blended is 0, using uniform distribution\n";
        // No valid orders - use uniform distribution
        for (int i = 0; i < 256; i++) {
            freqs[i] = 1;
        }
    }

    // Validation and debug output
    if (do_debug) {
        uint64_t final_sum = 0;
        for (int i = 0; i < 256; i++) {
            final_sum += freqs[i];
        }
        std::cerr << "  Final sum(freqs) = " << final_sum << " (expected ~" << OUTPUT_SCALE << ")\n";
        std::cerr << "  First 5 output freqs: [";
        for (int i = 0; i < 5; i++) {
            std::cerr << freqs[i];
            if (i < 4) std::cerr << ",";
        }
        std::cerr << "]\n";

        if (final_sum > OUTPUT_SCALE * 100) {
            std::cerr << "  *** ERROR: final_sum is " << (final_sum / OUTPUT_SCALE)
                      << "x too large! ***\n";
        }
        debug_count++;
    }
}

// ============================================================================
// Bit-Level Prediction (for future bit-by-bit encoding)
// ============================================================================

std::vector<Prediction> MultiOrderPPM::predict_bit(bool bit, int bit_pos) {
    // For now, treat as byte-level prediction
    // TODO: Implement true bit-level context modeling
    (void)bit;
    (void)bit_pos;
    return std::vector<Prediction>();
}

void MultiOrderPPM::update_bit(bool bit, int bit_pos) {
    // For now, no-op
    // TODO: Implement true bit-level updates
    (void)bit;
    (void)bit_pos;
}
