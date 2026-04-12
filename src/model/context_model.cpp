#include "model/context_model.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>
#include <iostream>

// ============================================================================
// Constructor / reset
// ============================================================================

ContextModel::ContextModel()
    : encoding_method_(EncodingMethod::SIMPLE),
      total0_(0),
      use_order2_(false),
      prev_prev_byte_(0),
      has_prev_prev_(false),
      o2_symbols_seen_(0),
      prev_byte_(0),
      has_prev_(false)
{
    std::memset(freq0_,       0, sizeof(freq0_));
    std::memset(freq1_,       0, sizeof(freq1_));
    std::memset(total1_,      0, sizeof(total1_));
    std::memset(seen1_,       0, sizeof(seen1_));
    std::memset(singleton1_,  0, sizeof(singleton1_));
    std::memset(ctx_exists_,  0, sizeof(ctx_exists_));
}

void ContextModel::reset() {
    has_prev_ = false;
    has_prev_prev_ = false;
    o2_symbols_seen_ = 0;
}

// ============================================================================
// Order-2: enable, direct-address helpers, rescale
// ============================================================================

void ContextModel::enable_order2() {
    use_order2_ = true;
    freq2_data_.assign(O2_CONTEXT_COUNT * EXTENDED_ALPHABET, 0);
    total2_.assign(O2_CONTEXT_COUNT, 0);
    singleton2_.assign(O2_CONTEXT_COUNT, 0);
    ctx2_exists_.assign(O2_CONTEXT_COUNT, 0);
    has_prev_prev_ = false;
    prev_prev_byte_ = 0;
}

uint16_t ContextModel::order2_context_key() const {
    return static_cast<uint16_t>((static_cast<uint16_t>(prev_prev_byte_) << 8) | prev_byte_);
}

uint32_t* ContextModel::freq2_ptr(uint16_t key) {
    return &freq2_data_[static_cast<size_t>(key) * EXTENDED_ALPHABET];
}

const uint32_t* ContextModel::freq2_ptr(uint16_t key) const {
    return &freq2_data_[static_cast<size_t>(key) * EXTENDED_ALPHABET];
}

bool ContextModel::has_order2_context() const {
    if (!use_order2_ || !has_prev_prev_ || !has_prev_) return false;
    if (o2_symbols_seen_ < O2_WARMUP_SIZE) return false;
    return ctx2_exists_[order2_context_key()] != 0;
}

uint32_t ContextModel::get_order2_total() const {
    return total2_[order2_context_key()];
}

const uint32_t* ContextModel::get_order2_freq_ptr() const {
    return freq2_ptr(order2_context_key());
}

void ContextModel::rescale_order2(uint16_t key) {
    uint32_t* f = freq2_ptr(key);
    total2_[key]     = 0;
    singleton2_[key] = 0;
    for (int s = 0; s < 256; s++) {
        if (f[s] > 0) {
            f[s] = (f[s] + 1) >> 1;
            if (f[s] == 1) singleton2_[key]++;
            total2_[key] += f[s];
        }
    }
    f[256] = std::max(1u, singleton2_[key]);
    total2_[key] += f[256];
}

int ContextModel::find_symbol_and_get_range_o2(uint32_t target,
                                                uint32_t& lo, uint32_t& hi,
                                                uint32_t& total) const {
    uint16_t key = order2_context_key();
    const uint32_t* f = freq2_ptr(key);
    total = total2_[key];
    uint32_t cum = 0;
    for (int s = 0; s < 257; s++) {
        uint32_t fs = f[s];
        if (fs) {
            uint32_t next = cum + fs;
            if (next > target) {
                lo = cum;
                hi = next;
                return s;
            }
            cum = next;
        }
    }
    return -1;
}

bool ContextModel::get_o2_byte_range(uint8_t byte, uint32_t& lo, uint32_t& hi, uint32_t& total) const {
    uint16_t key = order2_context_key();
    const uint32_t* f2 = freq2_ptr(key);
    if (!f2[byte]) return false;
    total = total2_[key];
    uint32_t cum = 0;
    for (int s = 0; s < byte; s++) cum += f2[s];
    lo = cum;
    hi = cum + f2[byte];
    return true;
}

void ContextModel::get_o2_escape_range(uint32_t& lo, uint32_t& hi, uint32_t& total) const {
    uint16_t key = order2_context_key();
    const uint32_t* f2 = freq2_ptr(key);
    total = total2_[key];
    lo    = total - f2[ESCAPE_SYMBOL];
    hi    = total;
}

// ============================================================================
// Initialisation
// ============================================================================

void ContextModel::init_adaptive() {
    // Order-0: all 256 bytes + EOF initialised to freq 1
    for (int i = 0; i < 256; i++) freq0_[i] = 1;
    freq0_[256] = 0;   // escape never in order-0
    freq0_[257] = 1;   // EOF
    total0_ = 257;

    // Order-1: clear everything
    std::memset(freq1_,      0, sizeof(freq1_));
    std::memset(total1_,     0, sizeof(total1_));
    std::memset(seen1_,      0, sizeof(seen1_));
    std::memset(singleton1_, 0, sizeof(singleton1_));
    std::memset(ctx_exists_, 0, sizeof(ctx_exists_));
    has_prev_ = false;
    has_prev_prev_ = false;
    o2_symbols_seen_ = 0;

    // Order-2: clear if enabled
    if (use_order2_) {
        std::fill(freq2_data_.begin(), freq2_data_.end(), 0);
        std::fill(total2_.begin(), total2_.end(), 0);
        std::fill(singleton2_.begin(), singleton2_.end(), 0);
        std::fill(ctx2_exists_.begin(), ctx2_exists_.end(), 0);
    }
}

void ContextModel::init_adaptive_with_warmup(const std::vector<uint8_t>& data, size_t warmup_size) {
    init_adaptive();

    size_t scan = std::min(warmup_size, data.size());
    for (size_t i = 0; i < scan; i++) {
        freq0_[data[i]] += 2;
        total0_ += 2;
    }

    std::cout << "Adaptive context model initialized with " << scan
              << " byte warmup" << std::endl;
}

void ContextModel::build_from_data(const std::vector<uint8_t>& data) {
    init_adaptive();
    for (size_t i = 0; i < data.size(); i++) {
        update_frequencies(data[i]);
        update_history(data[i]);
    }
    reset();
    std::cout << "Context model built successfully" << std::endl;
    print_statistics();
}

// ============================================================================
// History
// ============================================================================

void ContextModel::update_history(uint8_t byte) {
    if (use_order2_) {
        prev_prev_byte_ = prev_byte_;
        has_prev_prev_  = has_prev_;
        o2_symbols_seen_++;
    }
    prev_byte_ = byte;
    has_prev_  = true;
}

// ============================================================================
// Frequency update  (called BEFORE update_history for the same byte)
// ============================================================================

void ContextModel::update_frequencies(uint8_t byte) {
    // ── Order-0 update ───────────────────────────────────────────────────
    freq0_[byte]++;
    total0_++;

    // ── Order-1 update ───────────────────────────────────────────────────
    if (!has_prev_) return;

    uint8_t ctx = prev_byte_;

    if (!ctx_exists_[ctx]) {
        // First byte ever seen in this context — it is a singleton
        ctx_exists_[ctx]    = true;
        freq1_[ctx][byte]   = 1;
        freq1_[ctx][256]    = 1;   // escape = max(1, singleton_count=1)
        seen1_[ctx]         = 1;
        singleton1_[ctx]    = 1;
        total1_[ctx]        = 2;   // byte(1) + escape(1)
        return;
    }

    if (freq1_[ctx][byte] == 0) {
        // New symbol — becomes a singleton; update escape with PPMD estimator
        uint32_t old_esc = std::max(1u, singleton1_[ctx]);
        seen1_[ctx]++;
        singleton1_[ctx]++;
        uint32_t new_esc = std::max(1u, singleton1_[ctx]);

        freq1_[ctx][byte]  = 1;
        freq1_[ctx][256]   = new_esc;
        total1_[ctx]      += 1 + (new_esc - old_esc);
    } else {
        if (freq1_[ctx][byte] == 1) {
            // Singleton becoming non-singleton — escape drops
            uint32_t old_esc = std::max(1u, singleton1_[ctx]);
            if (singleton1_[ctx] > 0) singleton1_[ctx]--;
            uint32_t new_esc = std::max(1u, singleton1_[ctx]);

            freq1_[ctx][byte]++;
            freq1_[ctx][256]  = new_esc;
            total1_[ctx]     += 1 + (int32_t)(new_esc - old_esc);
        } else {
            // Existing non-singleton symbol — no change to escape
            freq1_[ctx][byte]++;
            total1_[ctx]++;
        }
    }

    if (total1_[ctx] > RESCALE_THRESH) { rescale_context(ctx); }

    // ── Order-2 update ───────────────────────────────────────────────────
    if (!use_order2_ || !has_prev_prev_) return;

    uint16_t key = order2_context_key();
    uint32_t* f2 = freq2_ptr(key);

    if (!ctx2_exists_[key]) {
        ctx2_exists_[key]   = 1;
        f2[byte]            = 1;
        f2[256]             = 1;   // escape
        singleton2_[key]    = 1;
        total2_[key]        = 2;
        return;
    }

    if (f2[byte] == 0) {
        uint32_t old_esc = std::max(1u, singleton2_[key]);
        singleton2_[key]++;
        uint32_t new_esc = std::max(1u, singleton2_[key]);
        f2[byte]  = 1;
        f2[256]   = new_esc;
        total2_[key] += 1 + (new_esc - old_esc);
    } else {
        if (f2[byte] == 1) {
            uint32_t old_esc = std::max(1u, singleton2_[key]);
            if (singleton2_[key] > 0) singleton2_[key]--;
            uint32_t new_esc = std::max(1u, singleton2_[key]);
            f2[byte]++;
            f2[256]   = new_esc;
            total2_[key] += 1 + static_cast<int32_t>(new_esc - old_esc);
        } else {
            f2[byte]++;
            total2_[key]++;
        }
    }

    if (total2_[key] > RESCALE_THRESH) { rescale_order2(key); }
}

// ============================================================================
// rescale_context — halve all Order-1 counts when total exceeds RESCALE_THRESH
// ============================================================================

void ContextModel::rescale_context(uint8_t ctx) {
    total1_[ctx]     = 0;
    singleton1_[ctx] = 0;

    for (int s = 0; s < 256; s++) {
        if (freq1_[ctx][s] > 0) {
            freq1_[ctx][s] = (freq1_[ctx][s] + 1) >> 1;  // halve, round up (keeps ≥ 1)
            if (freq1_[ctx][s] == 1) singleton1_[ctx]++;
            total1_[ctx] += freq1_[ctx][s];
        }
    }
    // Recompute escape using PPMD singleton estimator
    freq1_[ctx][256] = std::max(1u, singleton1_[ctx]);
    total1_[ctx]    += freq1_[ctx][256];
}

// ============================================================================
// update_order1_only — update exclusion variant
// Called after an escape: add symbol to Order-1 context only, skip Order-0.
// ============================================================================

void ContextModel::update_order1_only(uint8_t byte) {
    // Skip Order-0 update intentionally (update exclusion)

    if (!has_prev_) return;

    uint8_t ctx = prev_byte_;

    if (!ctx_exists_[ctx]) {
        ctx_exists_[ctx]    = true;
        freq1_[ctx][byte]   = 1;
        freq1_[ctx][256]    = 1;
        seen1_[ctx]         = 1;
        singleton1_[ctx]    = 1;
        total1_[ctx]        = 2;
        return;
    }

    if (freq1_[ctx][byte] == 0) {
        uint32_t old_esc = std::max(1u, singleton1_[ctx]);
        seen1_[ctx]++;
        singleton1_[ctx]++;
        uint32_t new_esc = std::max(1u, singleton1_[ctx]);

        freq1_[ctx][byte]  = 1;
        freq1_[ctx][256]   = new_esc;
        total1_[ctx]      += 1 + (new_esc - old_esc);
    } else {
        if (freq1_[ctx][byte] == 1) {
            uint32_t old_esc = std::max(1u, singleton1_[ctx]);
            if (singleton1_[ctx] > 0) singleton1_[ctx]--;
            uint32_t new_esc = std::max(1u, singleton1_[ctx]);

            freq1_[ctx][byte]++;
            freq1_[ctx][256]  = new_esc;
            total1_[ctx]     += 1 + (int32_t)(new_esc - old_esc);
        } else {
            freq1_[ctx][byte]++;
            total1_[ctx]++;
        }
    }
}

// ============================================================================
// Internal helper: sum freq[0..symbol-1]
// ============================================================================

uint32_t ContextModel::cumulative_before(const uint32_t* freq, int symbol) const {
    uint32_t cum = 0;
    for (int s = 0; s < symbol; s++) cum += freq[s];
    return cum;
}

// ============================================================================
// get_symbol_range
// ============================================================================

void ContextModel::get_symbol_range(int order, int symbol,
                                    uint32_t& low, uint32_t& high,
                                    uint32_t& total) const {
    if (order == 0) {
        uint32_t cum = cumulative_before(freq0_, symbol);
        low   = cum;
        high  = cum + freq0_[symbol];
        total = total0_;
    } else if (order == 1) {
        const uint32_t* f = freq1_[prev_byte_];
        total = total1_[prev_byte_];
        // Fast path: escape symbol (256) is always last non-zero symbol in order-1.
        // Its cumulative = total - f[ESCAPE_SYMBOL], avoiding O(256) prefix sum.
        if (symbol == ESCAPE_SYMBOL) {
            low  = total - f[ESCAPE_SYMBOL];
            high = total;
            return;
        }
        uint32_t cum = cumulative_before(f, symbol);
        low   = cum;
        high  = cum + f[symbol];
    } else {
        const uint32_t* f = get_order2_freq_ptr();
        total = total2_[order2_context_key()];
        if (symbol == ESCAPE_SYMBOL) {
            low  = total - f[ESCAPE_SYMBOL];
            high = total;
            return;
        }
        uint32_t cum = cumulative_before(f, symbol);
        low   = cum;
        high  = cum + f[symbol];
    }
}

// ============================================================================
// find_symbol
// ============================================================================

int ContextModel::find_symbol(int order, uint32_t cum_freq) const {
    if (order == 0) {
        uint32_t cum = 0;
        for (int s = 0; s < 258; s++) {
            uint32_t f = freq0_[s];
            if (f && cum + f > cum_freq) return s;
            cum += f;
        }
        return -1;
    } else if (order == 1) {
        const uint32_t* f = freq1_[prev_byte_];
        uint32_t cum = 0;
        for (int s = 0; s < 257; s++) {  // 0-255 bytes + 256 escape
            if (f[s] && cum + f[s] > cum_freq) return s;
            cum += f[s];
        }
        return -1;
    } else {
        uint32_t lo = 0;
        uint32_t hi = 0;
        uint32_t total = 0;
        return find_symbol_and_get_range_o2(cum_freq, lo, hi, total);
    }
}

// ============================================================================
// encode_symbol
// ============================================================================

std::vector<ContextModel::EncodingStep> ContextModel::encode_symbol(uint8_t byte) {
    if (encoding_method_ == EncodingMethod::SIMPLE)
        return encode_symbol_simple(byte);
    return encode_symbol_with_exclusions(byte);
}

std::vector<ContextModel::EncodingStep> ContextModel::encode_symbol_simple(uint8_t byte) {
    std::vector<EncodingStep> steps;
    steps.reserve(3);
    const uint32_t* o2_freq = nullptr;
    const bool has_o2 = has_order2_context();

    if (has_o2) {
        o2_freq = get_order2_freq_ptr();
        EncodingStep s;
        s.symbol = byte;
        if (get_o2_byte_range(byte, s.cum_freq_low, s.cum_freq_high, s.total_freq)) {
            steps.push_back(s);
            return steps;
        }

        EncodingStep esc;
        esc.symbol = ESCAPE_SYMBOL;
        get_o2_escape_range(esc.cum_freq_low, esc.cum_freq_high, esc.total_freq);
        steps.push_back(esc);
    }

    if (has_prev_ && ctx_exists_[prev_byte_] && freq1_[prev_byte_][byte] > 0 && (!has_o2 || o2_freq[byte] == 0)) {
        EncodingStep s;
        s.symbol = byte;
        if (has_o2) {
            get_order1_symbol_range_excl_ctx(byte, o2_freq,
                                             s.cum_freq_low,
                                             s.cum_freq_high,
                                             s.total_freq);
        } else {
            get_symbol_range(1, byte, s.cum_freq_low, s.cum_freq_high, s.total_freq);
        }
        steps.push_back(s);
        return steps;
    }

    if (has_prev_ && ctx_exists_[prev_byte_]) {
        EncodingStep esc;
        esc.symbol = ESCAPE_SYMBOL;
        if (has_o2) {
            uint32_t total_excl = get_order1_total_excl_ctx(o2_freq);
            find_symbol_and_get_range_order1_excl_ctx(total_excl - 1, o2_freq,
                                                      total_excl,
                                                      esc.cum_freq_low,
                                                      esc.cum_freq_high,
                                                      esc.total_freq);
        } else {
            get_symbol_range(1, ESCAPE_SYMBOL, esc.cum_freq_low, esc.cum_freq_high, esc.total_freq);
        }
        steps.push_back(esc);
    }

    EncodingStep s;
    s.symbol = byte;
    get_symbol_range(0, byte, s.cum_freq_low, s.cum_freq_high, s.total_freq);
    steps.push_back(s);

    return steps;
}

ContextModel::EncodeResult ContextModel::encode_symbol_fast(uint8_t byte) {
    EncodeResult res;
    res.count = 0;
    const uint32_t* o2_freq = nullptr;
    const bool has_o2 = has_order2_context();

    if (has_o2) {
        o2_freq = get_order2_freq_ptr();
        if (get_o2_byte_range(byte,
                              res.steps[0].cum_freq_low,
                              res.steps[0].cum_freq_high,
                              res.steps[0].total_freq)) {
            res.steps[0].symbol = byte;
            res.count = 1;
            return res;
        }

        get_o2_escape_range(res.steps[0].cum_freq_low,
                            res.steps[0].cum_freq_high,
                            res.steps[0].total_freq);
        res.steps[0].symbol = ESCAPE_SYMBOL;
        res.count = 1;
    }

    if (has_prev_ && ctx_exists_[prev_byte_] && freq1_[prev_byte_][byte] > 0 && (!has_o2 || o2_freq[byte] == 0)) {
        if (has_o2) {
            get_order1_symbol_range_excl_ctx(byte, o2_freq,
                                             res.steps[res.count].cum_freq_low,
                                             res.steps[res.count].cum_freq_high,
                                             res.steps[res.count].total_freq);
        } else {
            get_symbol_range(1, byte,
                             res.steps[res.count].cum_freq_low,
                             res.steps[res.count].cum_freq_high,
                             res.steps[res.count].total_freq);
        }
        res.steps[res.count].symbol = byte;
        res.count++;
        return res;
    }

    if (has_prev_ && ctx_exists_[prev_byte_]) {
        if (has_o2) {
            const uint32_t total_excl = get_order1_total_excl_ctx(o2_freq);
            find_symbol_and_get_range_order1_excl_ctx(total_excl - 1, o2_freq,
                                                      total_excl,
                                                      res.steps[res.count].cum_freq_low,
                                                      res.steps[res.count].cum_freq_high,
                                                      res.steps[res.count].total_freq);
        } else {
            get_symbol_range(1, ESCAPE_SYMBOL,
                             res.steps[res.count].cum_freq_low,
                             res.steps[res.count].cum_freq_high,
                             res.steps[res.count].total_freq);
        }
        res.steps[res.count].symbol = ESCAPE_SYMBOL;
        res.count++;
    }

    // Order-0 encode — use Method-C exclusions only after an Order-1 escape.
    const int idx = res.count;
    if (has_prev_ && ctx_exists_[prev_byte_] && encoding_method_ == EncodingMethod::METHOD_C) {
        // Fused single-pass: compute total AND find symbol simultaneously.
        // Method-C excludes symbols already present in the current O1 context.
            const uint32_t* ctx_freq = freq1_[prev_byte_];
            uint32_t cum = 0, tot = 0, sym_freq = 0;
            bool found = false;
            for (int s = 0; s < 258; s++) {
                if (ctx_freq[s]) continue;
                uint32_t f = freq0_[s];
                tot += f;
                if (s == byte) {
                    res.steps[idx].cum_freq_low = cum;
                    sym_freq = f;
                    found = true;
                } else if (!found) {
                    cum += f;
                }
            }
            if (found) {
                res.steps[idx].cum_freq_high = res.steps[idx].cum_freq_low + sym_freq;
                res.steps[idx].total_freq    = tot;
            }
    } else {
        get_symbol_range(0, byte,
                         res.steps[idx].cum_freq_low,
                         res.steps[idx].cum_freq_high,
                         res.steps[idx].total_freq);
    }
    res.steps[idx].symbol = byte;
    res.count++;

    return res;
}

// ============================================================================
// decode_symbol  (unused directly by decompressor — it calls find_symbol)
// ============================================================================

int ContextModel::decode_symbol(uint32_t cum_freq) {
    int order = has_order2_context() ? 2 : (has_prev_ ? 1 : 0);
    return find_symbol(order, cum_freq);
}

// ============================================================================
// Queries
// ============================================================================

uint32_t ContextModel::get_total_freq(int order) const {
    if (order == 0) return total0_;
    if (order == 1) {
        if (!has_prev_ || !ctx_exists_[prev_byte_]) return 0;
        return total1_[prev_byte_];
    }
    if (!has_order2_context()) return 0;
    return total2_[order2_context_key()];
}

bool ContextModel::has_symbol_in_context(int order, int symbol) const {
    if (order == 0) return freq0_[symbol] > 0;
    if (order == 1) {
        if (!has_prev_ || !ctx_exists_[prev_byte_]) return false;
        return freq1_[prev_byte_][symbol] > 0;
    }
    if (!has_order2_context()) return false;
    return freq2_ptr(order2_context_key())[symbol] > 0;
}

std::vector<int> ContextModel::get_context_symbols(int order) const {
    std::vector<int> syms;
    if (order == 0) {
        for (int s = 0; s < 258; s++)
            if (freq0_[s] > 0 && s != ESCAPE_SYMBOL && s != EOF_SYMBOL)
                syms.push_back(s);
    } else if (order == 1) {
        if (!has_prev_ || !ctx_exists_[prev_byte_]) return syms;
        const uint32_t* f = freq1_[prev_byte_];
        for (int s = 0; s < 256; s++)
            if (f[s] > 0) syms.push_back(s);
    } else {
        if (!has_order2_context()) return syms;
        const uint32_t* f = get_order2_freq_ptr();
        for (int s = 0; s < 256; s++)
            if (f[s] > 0) syms.push_back(s);
    }
    return syms;
}

void ContextModel::print_statistics() const {
    int count = 0;
    for (int i = 0; i < 256; i++) if (ctx_exists_[i]) count++;
    std::cout << "=== Context Model Statistics ===" << std::endl;
    std::cout << "Order-1 contexts active: " << count << " / 256" << std::endl;
    std::cout << "Order-0 total freq: " << total0_ << std::endl;
}

// ============================================================================
// Method-C exclusion helpers
// ============================================================================

std::vector<ContextModel::EncodingStep>
ContextModel::encode_symbol_with_exclusions(uint8_t byte) {
    std::vector<EncodingStep> steps;
    steps.reserve(3);
    const uint32_t* o2_freq = nullptr;
    const bool has_o2 = has_order2_context();

    if (has_o2) {
        o2_freq = get_order2_freq_ptr();
        EncodingStep s;
        s.symbol = byte;
        if (get_o2_byte_range(byte, s.cum_freq_low, s.cum_freq_high, s.total_freq)) {
            steps.push_back(s);
            return steps;
        }

        EncodingStep esc;
        esc.symbol = ESCAPE_SYMBOL;
        get_o2_escape_range(esc.cum_freq_low, esc.cum_freq_high, esc.total_freq);
        steps.push_back(esc);
    }

    if (has_prev_ && ctx_exists_[prev_byte_] && freq1_[prev_byte_][byte] > 0 && (!has_o2 || o2_freq[byte] == 0)) {
        EncodingStep s;
        s.symbol = byte;
        if (has_o2) {
            get_order1_symbol_range_excl_ctx(byte, o2_freq,
                                             s.cum_freq_low,
                                             s.cum_freq_high,
                                             s.total_freq);
        } else {
            get_symbol_range(1, byte, s.cum_freq_low, s.cum_freq_high, s.total_freq);
        }
        steps.push_back(s);
        return steps;
    }

    if (has_prev_ && ctx_exists_[prev_byte_]) {
        EncodingStep esc;
        esc.symbol = ESCAPE_SYMBOL;
        if (has_o2) {
            uint32_t total_excl = get_order1_total_excl_ctx(o2_freq);
            find_symbol_and_get_range_order1_excl_ctx(total_excl - 1, o2_freq,
                                                      total_excl,
                                                      esc.cum_freq_low,
                                                      esc.cum_freq_high,
                                                      esc.total_freq);
        } else {
            get_symbol_range(1, ESCAPE_SYMBOL, esc.cum_freq_low, esc.cum_freq_high, esc.total_freq);
        }
        steps.push_back(esc);

        EncodingStep s;
        s.symbol = byte;
        std::vector<int> excluded;
        const uint32_t* f1 = freq1_[prev_byte_];
        for (int i = 0; i < 256; i++) {
            if (f1[i] > 0) excluded.push_back(i);
        }
        get_symbol_range_with_exclusions(0, byte, excluded, s.cum_freq_low, s.cum_freq_high, s.total_freq);
        steps.push_back(s);
        return steps;
    }

    EncodingStep s;
    s.symbol = byte;
    get_symbol_range(0, byte, s.cum_freq_low, s.cum_freq_high, s.total_freq);
    steps.push_back(s);

    return steps;
}

void ContextModel::get_symbol_range_with_exclusions(int order, int symbol,
                                                     const std::vector<int>& excluded,
                                                     uint32_t& low, uint32_t& high,
                                                     uint32_t& total) const {
    // Build a temporary freq array excluding the listed symbols
    uint32_t tmp[258] = {};
    const uint32_t* src = (order == 0) ? freq0_ : freq1_[prev_byte_];
    uint32_t tot = 0;

    for (int s = 0; s < 258; s++) {
        bool excl = false;
        for (int e : excluded) if (e == s) { excl = true; break; }
        if (!excl) { tmp[s] = src[s]; tot += src[s]; }
    }

    uint32_t cum = cumulative_before(tmp, symbol);
    low   = cum;
    high  = cum + tmp[symbol];
    total = tot;
}

int ContextModel::find_symbol_and_get_range(int order, uint32_t target,
                                             uint32_t& lo, uint32_t& hi,
                                             uint32_t& total) const {
    if (order == 0) {
        total = total0_;
        uint32_t cum = 0;
        for (int s = 0; s < 258; s++) {
            uint32_t f = freq0_[s];
            if (f) {
                uint32_t next = cum + f;
                if (next > target) {
                    lo = cum;
                    hi = next;
                    return s;
                }
                cum = next;
            }
        }
        return -1;
    } else if (order == 1) {
        const uint32_t* f = freq1_[prev_byte_];
        total = total1_[prev_byte_];
        uint32_t cum = 0;
        for (int s = 0; s < 257; s++) {
            uint32_t fs = f[s];
            if (fs) {
                uint32_t next = cum + fs;
                if (next > target) {
                    lo = cum;
                    hi = next;
                    return s;
                }
                cum = next;
            }
        }
        return -1;
    } else {
        return find_symbol_and_get_range_o2(target, lo, hi, total);
    }
}

uint32_t ContextModel::get_order1_total_excl_ctx(const uint32_t* excluded_ctx_freq) const {
    uint32_t tot = 0;
    const uint32_t* f = freq1_[prev_byte_];
    for (int s = 0; s < 256; s++) {
        if (!excluded_ctx_freq[s]) tot += f[s];
    }
    tot += f[ESCAPE_SYMBOL];
    return tot;
}

bool ContextModel::get_order1_symbol_range_excl_ctx(int symbol,
                                                     const uint32_t* excluded_ctx_freq,
                                                     uint32_t& lo, uint32_t& hi,
                                                     uint32_t& total) const {
    total = get_order1_total_excl_ctx(excluded_ctx_freq);
    const uint32_t* f = freq1_[prev_byte_];
    if (symbol != ESCAPE_SYMBOL && excluded_ctx_freq[symbol]) return false;
    if (!f[symbol]) return false;

    uint32_t cum = 0;
    for (int s = 0; s < symbol; s++) {
        if (s < 256 && excluded_ctx_freq[s]) continue;
        cum += f[s];
    }

    lo = cum;
    hi = cum + f[symbol];
    return true;
}

int ContextModel::find_symbol_and_get_range_order1_excl_ctx(uint32_t target,
                                                             const uint32_t* excluded_ctx_freq,
                                                             uint32_t& lo, uint32_t& hi,
                                                             uint32_t& total) const {
    total = get_order1_total_excl_ctx(excluded_ctx_freq);
    return find_symbol_and_get_range_order1_excl_ctx(target, excluded_ctx_freq, total, lo, hi, total);
}

int ContextModel::find_symbol_and_get_range_order1_excl_ctx(uint32_t target,
                                                             const uint32_t* excluded_ctx_freq,
                                                             uint32_t precomputed_total,
                                                             uint32_t& lo, uint32_t& hi,
                                                             uint32_t& total) const {
    total = precomputed_total;
    const uint32_t* f = freq1_[prev_byte_];
    uint32_t cum = 0;
    for (int s = 0; s < 257; s++) {
        if (s < 256 && excluded_ctx_freq[s]) continue;
        uint32_t fs = f[s];
        if (fs) {
            uint32_t next = cum + fs;
            if (next > target) {
                lo = cum;
                hi = next;
                return s;
            }
            cum = next;
        }
    }
    return -1;
}

uint32_t ContextModel::get_order0_total_excl_ctx(const uint32_t* ctx_freq) const {
    uint32_t tot = 0;
    for (int s = 0; s < 258; s++) {
        if (!ctx_freq[s]) tot += freq0_[s];
    }
    return tot;
}

int ContextModel::find_symbol_and_get_range_excl_ctx(uint32_t target,
                                                      const uint32_t* ctx_freq,
                                                      uint32_t& lo, uint32_t& hi,
                                                      uint32_t& total) const {
    uint32_t tot = 0;
    for (int s = 0; s < 258; s++) {
        if (!ctx_freq[s]) tot += freq0_[s];
    }
    total = tot;
    uint32_t cum = 0;
    for (int s = 0; s < 258; s++) {
        if (ctx_freq[s]) continue;
        uint32_t f = freq0_[s];
        if (f) {
            uint32_t next = cum + f;
            if (next > target) {
                lo = cum;
                hi = next;
                return s;
            }
            cum = next;
        }
    }
    return -1;
}

int ContextModel::find_symbol_and_get_range_excl_ctx(uint32_t target,
                                                      const uint32_t* ctx_freq,
                                                      uint32_t precomputed_total,
                                                      uint32_t& lo, uint32_t& hi,
                                                      uint32_t& total) const {
    total = precomputed_total;
    uint32_t cum = 0;
    for (int s = 0; s < 258; s++) {
        if (ctx_freq[s]) continue;
        uint32_t f = freq0_[s];
        if (f) {
            uint32_t next = cum + f;
            if (next > target) {
                lo = cum;
                hi = next;
                return s;
            }
            cum = next;
        }
    }
    return -1;
}

int ContextModel::find_symbol_with_exclusions(int order, uint32_t cum_freq,
                                               const std::vector<int>& excluded) const {
    const uint32_t* src = (order == 0) ? freq0_ : freq1_[prev_byte_];
    uint32_t cum = 0;
    for (int s = 0; s < 258; s++) {
        bool excl = false;
        for (int e : excluded) if (e == s) { excl = true; break; }
        if (excl) continue;
        if (src[s] && cum + src[s] > cum_freq) return s;
        cum += src[s];
    }
    return -1;
}

uint32_t ContextModel::get_total_freq_with_exclusions(int order,
                                                       const std::vector<int>& excluded) const {
    const uint32_t* src = (order == 0) ? freq0_ : freq1_[prev_byte_];
    uint32_t tot = 0;
    for (int s = 0; s < 258; s++) {
        bool excl = false;
        for (int e : excluded) if (e == s) { excl = true; break; }
        if (!excl) tot += src[s];
    }
    return tot;
}
