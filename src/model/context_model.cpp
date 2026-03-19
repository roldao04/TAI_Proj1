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
      prev_byte_(0),
      has_prev_(false)
{
    std::memset(freq0_,      0, sizeof(freq0_));
    std::memset(freq1_,      0, sizeof(freq1_));
    std::memset(total1_,     0, sizeof(total1_));
    std::memset(seen1_,      0, sizeof(seen1_));
    std::memset(ctx_exists_, 0, sizeof(ctx_exists_));
}

void ContextModel::reset() {
    has_prev_ = false;
}

// ============================================================================
// Initialisation
// ============================================================================

void ContextModel::init_adaptive() {
    // Order-0: all 256 bytes + EOF initialised to freq 1 (matches original)
    for (int i = 0; i < 256; i++) freq0_[i] = 1;
    freq0_[256] = 0;   // escape never in order-0
    freq0_[257] = 1;   // EOF
    total0_ = 257;

    // Order-1: clear everything
    std::memset(freq1_,      0, sizeof(freq1_));
    std::memset(total1_,     0, sizeof(total1_));
    std::memset(seen1_,      0, sizeof(seen1_));
    std::memset(ctx_exists_, 0, sizeof(ctx_exists_));
    has_prev_ = false;
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
        // First byte ever seen in this context
        ctx_exists_[ctx]    = true;
        freq1_[ctx][byte]   = 1;
        freq1_[ctx][256]    = 1;   // escape freq = max(1, 1/4) = 1
        seen1_[ctx]         = 1;
        total1_[ctx]        = 2;   // byte(1) + escape(1)
        return;
    }

    if (freq1_[ctx][byte] == 0) {
        // New symbol in existing context — update escape too
        uint32_t old_esc = std::max(1u, seen1_[ctx] / 4);
        seen1_[ctx]++;
        uint32_t new_esc = std::max(1u, seen1_[ctx] / 4);

        freq1_[ctx][byte]  = 1;
        freq1_[ctx][256]   = new_esc;
        total1_[ctx]      += 1 + (new_esc - old_esc);
    } else {
        // Existing symbol
        freq1_[ctx][byte]++;
        total1_[ctx]++;
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
    } else {
        // order == 1
        const uint32_t* f = freq1_[prev_byte_];
        uint32_t cum = cumulative_before(f, symbol);
        low   = cum;
        high  = cum + f[symbol];
        total = total1_[prev_byte_];
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
    } else {
        // order == 1
        const uint32_t* f = freq1_[prev_byte_];
        uint32_t cum = 0;
        for (int s = 0; s < 257; s++) {  // 0-255 bytes + 256 escape
            if (f[s] && cum + f[s] > cum_freq) return s;
            cum += f[s];
        }
        return -1;
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
    steps.reserve(2);

    if (has_prev_ && ctx_exists_[prev_byte_] && freq1_[prev_byte_][byte] > 0) {
        // Symbol known in order-1 context — encode directly
        EncodingStep s;
        s.symbol = byte;
        get_symbol_range(1, byte, s.cum_freq_low, s.cum_freq_high, s.total_freq);
        steps.push_back(s);
    } else {
        // Encode escape in order-1 (if context exists), then byte in order-0
        if (has_prev_ && ctx_exists_[prev_byte_]) {
            EncodingStep esc;
            esc.symbol = ESCAPE_SYMBOL;
            get_symbol_range(1, ESCAPE_SYMBOL, esc.cum_freq_low, esc.cum_freq_high, esc.total_freq);
            steps.push_back(esc);
        }
        EncodingStep s;
        s.symbol = byte;
        get_symbol_range(0, byte, s.cum_freq_low, s.cum_freq_high, s.total_freq);
        steps.push_back(s);
    }

    return steps;
}

ContextModel::EncodeResult ContextModel::encode_symbol_fast(uint8_t byte) {
    EncodeResult res;
    res.count = 0;

    if (has_prev_ && ctx_exists_[prev_byte_] && freq1_[prev_byte_][byte] > 0) {
        // Direct order-1 encode
        get_symbol_range(1, byte,
                         res.steps[0].cum_freq_low,
                         res.steps[0].cum_freq_high,
                         res.steps[0].total_freq);
        res.steps[0].symbol = byte;
        res.count = 1;
    } else {
        if (has_prev_ && ctx_exists_[prev_byte_]) {
            // Escape in order-1
            get_symbol_range(1, ESCAPE_SYMBOL,
                             res.steps[0].cum_freq_low,
                             res.steps[0].cum_freq_high,
                             res.steps[0].total_freq);
            res.steps[0].symbol = ESCAPE_SYMBOL;
            res.count = 1;
        }
        // Order-0 encode
        get_symbol_range(0, byte,
                         res.steps[res.count].cum_freq_low,
                         res.steps[res.count].cum_freq_high,
                         res.steps[res.count].total_freq);
        res.steps[res.count].symbol = byte;
        res.count++;
    }

    return res;
}

// ============================================================================
// decode_symbol  (unused directly by decompressor — it calls find_symbol)
// ============================================================================

int ContextModel::decode_symbol(uint32_t cum_freq) {
    int order = has_prev_ ? 1 : 0;
    return find_symbol(order, cum_freq);
}

// ============================================================================
// Queries
// ============================================================================

uint32_t ContextModel::get_total_freq(int order) const {
    if (order == 0) return total0_;
    // order == 1
    if (!has_prev_ || !ctx_exists_[prev_byte_]) return 0;
    return total1_[prev_byte_];
}

bool ContextModel::has_symbol_in_context(int order, int symbol) const {
    if (order == 0) return freq0_[symbol] > 0;
    if (!has_prev_ || !ctx_exists_[prev_byte_]) return false;
    return freq1_[prev_byte_][symbol] > 0;
}

std::vector<int> ContextModel::get_context_symbols(int order) const {
    std::vector<int> syms;
    if (order == 0) {
        for (int s = 0; s < 258; s++)
            if (freq0_[s] > 0 && s != ESCAPE_SYMBOL && s != EOF_SYMBOL)
                syms.push_back(s);
    } else {
        if (!has_prev_ || !ctx_exists_[prev_byte_]) return syms;
        const uint32_t* f = freq1_[prev_byte_];
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
// Method-C stubs (not used in SIMPLE mode)
// ============================================================================

std::vector<ContextModel::EncodingStep>
ContextModel::encode_symbol_with_exclusions(uint8_t byte) {
    // Not used — fall back to simple
    return encode_symbol_simple(byte);
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
