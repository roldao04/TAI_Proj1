#ifndef CONTEXT_MODEL_H
#define CONTEXT_MODEL_H

#include <vector>
#include <cstdint>
#include <array>
#include <map>
#include <memory>

/*
 * Order-k Context Model with Escape Mechanism
 *
 * This model uses the previous k bytes as context to predict the next byte.
 * It implements a fallback chain: Order-2 → Order-1 → Order-0
 *
 * When a symbol hasn't been seen in a context, an escape symbol is encoded
 * and the model falls back to a lower order.
 *
 * Memory usage: ~260MB for Order-2 (65,536 contexts × ~4KB each)
 */

class ContextModel {
private:
    static const int ALPHABET_SIZE = 256;  // 256 bytes
    static const int ESCAPE_SYMBOL = 256;  // Escape to lower order
    static const int EOF_SYMBOL = 257;     // End of file marker
    static const int EXTENDED_ALPHABET = 258;  // 256 bytes + escape + EOF

    // Maximum order (k=2 means use previous 2 bytes as context)
    static const int MAX_ORDER = 2;

    // Context structure: maps symbol → frequency
    struct Context {
        std::map<int, uint32_t> frequencies;  // symbol → count
        uint32_t total_freq;

        // Cache for cumulative frequencies (optimization to avoid recomputing)
        std::vector<int> cached_symbols;           // Sorted list of symbols
        std::vector<uint32_t> cached_cumulative;   // Cumulative frequencies
        bool cumulative_dirty;                     // True if cache needs rebuild

        bool has_symbol(int symbol) const {
            return frequencies.find(symbol) != frequencies.end();
        }

        void invalidate_cache() {
            cumulative_dirty = true;
        }

        void ensure_cache_valid();  // Rebuild cache if dirty

        Context() : total_freq(0), cumulative_dirty(true) {}
    };

    // Order-2 contexts: 65,536 possible 2-byte contexts
    std::array<std::unique_ptr<Context>, 65536> order2_contexts;

    // Order-1 contexts: 256 possible 1-byte contexts
    std::array<std::unique_ptr<Context>, 256> order1_contexts;

    // Order-0 context: single context for all symbols (fallback)
    std::unique_ptr<Context> order0_context;

    // History buffer (stores last MAX_ORDER bytes)
    std::vector<uint8_t> history;

    // Helper methods
    uint32_t get_context_id_order2() const;
    uint32_t get_context_id_order1() const;

    Context* get_context(int order);
    void ensure_context_exists(int order);

    void build_cumulative_freq(const Context* ctx,
                              std::vector<int>& symbols,
                              std::vector<uint32_t>& cumulative) const;

public:
    ContextModel();

    // Build model from data (static mode - pre-scans data)
    void build_from_data(const std::vector<uint8_t>& data);

    // Initialize for adaptive mode (no pre-scanning)
    void init_adaptive();

    // Initialize for adaptive mode with warm start (scan first N bytes)
    void init_adaptive_with_warmup(const std::vector<uint8_t>& data, size_t warmup_size = 8192);

    // Encode a symbol: returns list of (symbol, cum_low, cum_high, total) tuples
    // May encode multiple symbols (escapes) before encoding the actual byte
    struct EncodingStep {
        int symbol;
        uint32_t cum_freq_low;
        uint32_t cum_freq_high;
        uint32_t total_freq;
    };
    std::vector<EncodingStep> encode_symbol(uint8_t byte);

    // PPM Method C: Encode with exclusions
    std::vector<EncodingStep> encode_symbol_with_exclusions(uint8_t byte);

    // Decode a symbol: may need multiple steps (escapes) to get actual byte
    // Returns -1 if escape, >= 0 if actual symbol, -2 if EOF
    int decode_symbol(uint32_t cum_freq);

    // Update history after encoding/decoding a symbol
    void update_history(uint8_t byte);

    // Update frequencies adaptively after encoding/decoding a symbol
    void update_frequencies(uint8_t byte);

    // Reset history (at start of new file)
    void reset();

    // Get symbol range for encoding at a specific order
    void get_symbol_range(int order, int symbol,
                         uint32_t& cum_freq_low,
                         uint32_t& cum_freq_high,
                         uint32_t& total_freq) const;

    // Get symbol range with exclusions (PPM Method C)
    void get_symbol_range_with_exclusions(int order, int symbol,
                                         const std::vector<int>& excluded_symbols,
                                         uint32_t& cum_freq_low,
                                         uint32_t& cum_freq_high,
                                         uint32_t& total_freq) const;

    // Find symbol from cumulative frequency at a specific order
    int find_symbol(int order, uint32_t cum_freq) const;

    // Find symbol with exclusions (PPM Method C)
    int find_symbol_with_exclusions(int order, uint32_t cum_freq,
                                   const std::vector<int>& excluded_symbols) const;

    // Check if symbol exists in current context at given order
    bool has_symbol_in_context(int order, int symbol) const;

    // Get total frequency for current context at given order
    uint32_t get_total_freq(int order) const;

    // Get total frequency with exclusions (PPM Method C)
    uint32_t get_total_freq_with_exclusions(int order, const std::vector<int>& excluded_symbols) const;

    // Get current history size (for decoder to match encoder logic)
    size_t get_history_size() const { return history.size(); }

    // Get all symbols in a context (for PPM exclusions)
    std::vector<int> get_context_symbols(int order) const;

    // Statistics
    void print_statistics() const;
};

#endif // CONTEXT_MODEL_H
