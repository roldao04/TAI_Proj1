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
 * SOLID Principles Applied:
 * - SRP: Each method has single responsibility
 * - OCP: Extensible encoding strategies
 * - LSP: Consistent interfaces across modes
 * - ISP: Separated interfaces for different concerns
 * - DIP: Depends on abstract encoding strategy
 *
 * Memory usage: ~260MB for Order-2 (65,536 contexts × ~4KB each)
 */

class ContextModel {
public:
    // EncodingStep struct: represents one encoding operation (must be public and declared first)
    struct EncodingStep {
        int symbol;
        uint32_t cum_freq_low;
        uint32_t cum_freq_high;
        uint32_t total_freq;
    };

private:
    static const int ALPHABET_SIZE = 256;  // 256 bytes
    static const int ESCAPE_SYMBOL = 256;  // Escape to lower order
    static const int EOF_SYMBOL = 257;     // End of file marker
    static const int EXTENDED_ALPHABET = 258;  // 256 bytes + escape + EOF

    // Maximum order (k=2 means use previous 2 bytes as context)
    static const int MAX_ORDER = 2;

    // Encoding method configuration (OCP: Open for extension)
    enum class EncodingMethod {
        SIMPLE,      // No exclusions, faster (Order-1 only)
        METHOD_C     // Full PPM Method C with exclusions
    };

    EncodingMethod encoding_method_;

    // Context structure: maps symbol → frequency (SRP: encapsulated context management)
    struct Context {
        std::map<int, uint32_t> frequencies;  // symbol → count
        uint32_t total_freq;

        // Cache for cumulative frequencies (optimization to avoid recomputing)
        std::vector<int> cached_symbols;           // Sorted list of symbols
        std::vector<uint32_t> cached_cumulative;   // Cumulative frequencies
        bool cumulative_dirty;                     // True if cache needs rebuild

        Context() : total_freq(0), cumulative_dirty(true) {}

        // SRP: Single responsibility for symbol checking
        bool has_symbol(int symbol) const {
            return frequencies.find(symbol) != frequencies.end();
        }

        // SRP: Single responsibility for cache management
        void invalidate_cache() {
            cumulative_dirty = true;
        }

        void ensure_cache_valid();  // Rebuild cache if dirty
    };

    // Order-2 contexts: DISABLED - no benefit over Order-1, wastes ~260MB memory
    // Benchmark results showed identical compression ratios with worse performance
    // std::array<std::unique_ptr<Context>, 65536> order2_contexts;

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

    // SRP: Separate encoding strategies
    std::vector<EncodingStep> encode_symbol_simple(uint8_t byte);
    std::vector<EncodingStep> encode_symbol_with_exclusions(uint8_t byte);

public:
    ContextModel();

    // Configuration methods (OCP: Open for extension, closed for modification)
    void set_encoding_method_simple() { encoding_method_ = EncodingMethod::SIMPLE; }
    void set_encoding_method_ppm_c() { encoding_method_ = EncodingMethod::METHOD_C; }
    bool is_using_exclusions() const { return encoding_method_ == EncodingMethod::METHOD_C; }

    // Build model from data (static mode - pre-scans data)
    void build_from_data(const std::vector<uint8_t>& data);

    // Initialize for adaptive mode (no pre-scanning)
    void init_adaptive();

    // Initialize for adaptive mode with warm start (scan first N bytes)
    void init_adaptive_with_warmup(const std::vector<uint8_t>& data, size_t warmup_size = 8192);

    // Core API: Encode a symbol (LSP: Consistent interface regardless of encoding method)
    // Returns list of encoding steps (may include escapes)
    std::vector<EncodingStep> encode_symbol(uint8_t byte);

    // Decode a symbol: may need multiple steps (escapes) to get actual byte
    // Returns -1 if escape, >= 0 if actual symbol, -2 if EOF
    int decode_symbol(uint32_t cum_freq);

    // Model update operations (SRP: grouped by responsibility)
    void update_history(uint8_t byte);
    void update_frequencies(uint8_t byte);
    void reset();

    // ISP: Symbol range queries interface
    void get_symbol_range(int order, int symbol,
                         uint32_t& cum_freq_low,
                         uint32_t& cum_freq_high,
                         uint32_t& total_freq) const;

    void get_symbol_range_with_exclusions(int order, int symbol,
                                         const std::vector<int>& excluded_symbols,
                                         uint32_t& cum_freq_low,
                                         uint32_t& cum_freq_high,
                                         uint32_t& total_freq) const;

    // ISP: Symbol finding interface
    int find_symbol(int order, uint32_t cum_freq) const;
    int find_symbol_with_exclusions(int order, uint32_t cum_freq,
                                   const std::vector<int>& excluded_symbols) const;

    // ISP: Context query interface
    bool has_symbol_in_context(int order, int symbol) const;
    uint32_t get_total_freq(int order) const;
    uint32_t get_total_freq_with_exclusions(int order, const std::vector<int>& excluded_symbols) const;
    size_t get_history_size() const { return history.size(); }
    std::vector<int> get_context_symbols(int order) const;

    // Statistics and debugging
    void print_statistics() const;
};

#endif // CONTEXT_MODEL_H
