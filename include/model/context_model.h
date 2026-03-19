#ifndef CONTEXT_MODEL_H
#define CONTEXT_MODEL_H

#include <vector>
#include <cstdint>
#include <array>

/*
 * Order-1 Adaptive Context Model — Flat Array Implementation
 *
 * Replaces the original std::map + dirty-cache design with flat 2D arrays.
 *
 * Previous design bottleneck:
 *   - std::map<int,uint32_t> per context → tree traversal + pointer chasing
 *   - Dirty cache rebuilt via sort + cumulative scan on every update
 *   - Result: ~6.5s for 1MB file (99% of time in this class)
 *
 * New design:
 *   - freq0[258]         — Order-0 flat frequency array (L1 cache: 1 KB)
 *   - freq1[256][258]    — Order-1 flat array [context][symbol] (~258 KB, L2/L3)
 *   - get_symbol_range() — linear prefix sum over 258 elements (vectorizable)
 *   - find_symbol()      — linear scan over 258 elements (vectorizable)
 *   - update()           — single array write + counter increment
 *
 * Symbol space:
 *   0-255  = byte values
 *   256    = ESCAPE (fall back to lower order)
 *   257    = EOF    (only used in Order-0 static mode, not adaptive)
 */

class ContextModel {
public:
    struct EncodingStep {
        int      symbol;
        uint32_t cum_freq_low;
        uint32_t cum_freq_high;
        uint32_t total_freq;
    };

    // Fixed-size result: avoids heap allocation in the hot encode loop.
    // Always holds 1 or 2 steps (1 = direct order-1, 2 = escape + order-0).
    struct EncodeResult {
        EncodingStep steps[2];
        int count;
    };

    static const int ALPHABET_SIZE   = 256;
    static const int ESCAPE_SYMBOL   = 256;
    static const int EOF_SYMBOL      = 257;
    static const int EXTENDED_ALPHABET = 258;  // bytes + escape + EOF
    static const int MAX_ORDER       = 2;      // kept for interface compat

    // Rescaling threshold: when an Order-1 context total exceeds this,
    // halve all counts to keep the model adaptive to recent statistics.
    static const uint32_t RESCALE_THRESH = 8192;

private:
    enum class EncodingMethod { SIMPLE, METHOD_C };
    EncodingMethod encoding_method_;

    // ── Order-0 ──────────────────────────────────────────────────────────
    uint32_t freq0_[258];   // freq0_[s] = frequency of symbol s
    uint32_t total0_;

    // ── Order-1 ──────────────────────────────────────────────────────────
    uint32_t freq1_[256][258]; // freq1_[ctx][s]
    uint32_t total1_[256];
    uint32_t seen1_[256];      // unique non-escape bytes seen per context
    uint32_t singleton1_[256]; // symbols with freq == 1 (PPMD escape estimator)
    bool     ctx_exists_[256]; // has this context been initialised?

    // ── History ───────────────────────────────────────────────────────────
    uint8_t  prev_byte_;
    bool     has_prev_;

    // ── Internal helpers ─────────────────────────────────────────────────
    std::vector<EncodingStep> encode_symbol_simple(uint8_t byte);
    std::vector<EncodingStep> encode_symbol_with_exclusions(uint8_t byte);

    uint32_t cumulative_before(const uint32_t* freq, int symbol) const;
    void rescale_context(uint8_t ctx);  // halve all counts in one Order-1 context

public:
    ContextModel();

    void set_encoding_method_simple() { encoding_method_ = EncodingMethod::SIMPLE; }
    void set_encoding_method_ppm_c()  { encoding_method_ = EncodingMethod::METHOD_C; }
    bool is_using_exclusions() const  { return encoding_method_ == EncodingMethod::METHOD_C; }

    void build_from_data(const std::vector<uint8_t>& data);
    void init_adaptive();
    void init_adaptive_with_warmup(const std::vector<uint8_t>& data, size_t warmup_size = 8192);

    std::vector<EncodingStep> encode_symbol(uint8_t byte);
    EncodeResult encode_symbol_fast(uint8_t byte);

    int decode_symbol(uint32_t cum_freq);

    void update_history(uint8_t byte);
    void update_frequencies(uint8_t byte);
    // Update exclusion: only update Order-1 (not Order-0) after an escape.
    // Used when a symbol was encoded/decoded via Order-0 fallback from Order-1.
    void update_order1_only(uint8_t byte);
    void reset();

    void get_symbol_range(int order, int symbol,
                          uint32_t& cum_freq_low,
                          uint32_t& cum_freq_high,
                          uint32_t& total_freq) const;

    // Method-C exclusion variants (unused in SIMPLE mode, kept for interface compat)
    void get_symbol_range_with_exclusions(int order, int symbol,
                                          const std::vector<int>& excluded,
                                          uint32_t& low, uint32_t& high,
                                          uint32_t& total) const;

    int find_symbol(int order, uint32_t cum_freq) const;
    // Fused decode helper: finds symbol AND fills lo/hi/total in one scan (2× faster than
    // calling find_symbol + get_symbol_range separately).
    int find_symbol_and_get_range(int order, uint32_t target,
                                  uint32_t& lo, uint32_t& hi, uint32_t& total) const;
    int find_symbol_with_exclusions(int order, uint32_t cum_freq,
                                    const std::vector<int>& excluded) const;

    // Fast inline accessors used by the hot decode loop
    bool     has_order1_context()  const { return has_prev_ && ctx_exists_[prev_byte_]; }
    uint32_t get_order0_total()    const { return total0_; }
    uint32_t get_order1_total()    const { return total1_[prev_byte_]; }

    // Method C: exclusion-based helpers (no heap allocation)
    // Returns pointer to current Order-1 context freq array (valid while prev_byte_ unchanged)
    const uint32_t* get_order1_freq_ptr() const { return freq1_[prev_byte_]; }
    uint32_t get_order0_total_excl_ctx(const uint32_t* ctx_freq) const;
    // Fused find+range for Order-0 excluding symbols seen in ctx_freq
    int find_symbol_and_get_range_excl_ctx(uint32_t target, const uint32_t* ctx_freq,
                                           uint32_t& lo, uint32_t& hi, uint32_t& total) const;

    bool     has_symbol_in_context(int order, int symbol) const;
    uint32_t get_total_freq(int order) const;
    uint32_t get_total_freq_with_exclusions(int order, const std::vector<int>& excluded) const;
    size_t   get_history_size() const { return has_prev_ ? 1 : 0; }
    std::vector<int> get_context_symbols(int order) const;

    void print_statistics() const;
};

#endif // CONTEXT_MODEL_H
