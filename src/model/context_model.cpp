#include "model/context_model.h"
#include <algorithm>
#include <stdexcept>
#include <iostream>

// Define static constants
const int ContextModel::ALPHABET_SIZE;
const int ContextModel::ESCAPE_SYMBOL;
const int ContextModel::EOF_SYMBOL;
const int ContextModel::EXTENDED_ALPHABET;
const int ContextModel::MAX_ORDER;

ContextModel::ContextModel() {
    order0_context = std::make_unique<Context>();
    encoding_method_ = EncodingMethod::SIMPLE;  // Default to simple mode for speed
    reset();
}

void ContextModel::reset() {
    history.clear();
}

uint32_t ContextModel::get_context_id_order2() const {
    if (history.size() < 2) return 0;
    return ((uint32_t)history[history.size() - 2] << 8) | history[history.size() - 1];
}

uint32_t ContextModel::get_context_id_order1() const {
    if (history.size() < 1) return 0;
    return history[history.size() - 1];
}

ContextModel::Context* ContextModel::get_context(int order) {
    if (order == 0) {
        return order0_context.get();
    } else if (order == 1) {
        if (history.size() < 1) return nullptr;
        uint32_t ctx_id = get_context_id_order1();
        return order1_contexts[ctx_id].get();
    }
    // Order-2 removed - no benefit over Order-1
    return nullptr;
}

void ContextModel::ensure_context_exists(int order) {
    if (order == 0) {
        // Order-0 always exists
        return;
    } else if (order == 1) {
        if (history.size() < 1) return;
        uint32_t ctx_id = get_context_id_order1();
        if (!order1_contexts[ctx_id]) {
            // Lazy context creation threshold: only create if context byte seen enough times
            uint8_t context_byte = history[history.size() - 1];
            if (order0_context->frequencies.find(context_byte) != order0_context->frequencies.end() &&
                order0_context->frequencies[context_byte] >= 3) {
                order1_contexts[ctx_id] = std::make_unique<Context>();
            }
        }
    }
    // Order-2 removed - no benefit over Order-1
}

void ContextModel::init_adaptive() {
    // Initialize Order-0 with all symbols having frequency 1 (uniform prior)
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        order0_context->frequencies[i] = 1;
    }
    order0_context->frequencies[EOF_SYMBOL] = 1;
    order0_context->total_freq = ALPHABET_SIZE + 1;
    order0_context->invalidate_cache();  // Mark cache as dirty

    // Reset history
    reset();

    // Log initialization mode
    std::cout << "Adaptive context model initialized (";
    std::cout << (encoding_method_ == EncodingMethod::SIMPLE ? "Simple" : "PPM Method C");
    std::cout << " mode)" << std::endl;
}

void ContextModel::init_adaptive_with_warmup(const std::vector<uint8_t>& data, size_t warmup_size) {
    // Initialize Order-0 with small base frequency for unseen symbols
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        order0_context->frequencies[i] = 1;
    }
    order0_context->frequencies[EOF_SYMBOL] = 1;
    order0_context->total_freq = ALPHABET_SIZE + 1;

    // Warm start: scan first warmup_size bytes to build better priors
    size_t scan_size = std::min(warmup_size, data.size());

    for (size_t i = 0; i < scan_size; i++) {
        uint8_t byte = data[i];
        order0_context->frequencies[byte] += 2;  // Weight warmup samples higher
        order0_context->total_freq += 2;
    }

    order0_context->invalidate_cache();

    // Reset history
    reset();

    std::cout << "Adaptive context model initialized with " << scan_size
              << " byte warmup" << std::endl;
}

void ContextModel::build_from_data(const std::vector<uint8_t>& data) {
    // Initialize Order-0 with all symbols having frequency 1
    for (int i = 0; i < ALPHABET_SIZE; i++) {
        order0_context->frequencies[i] = 1;
    }
    order0_context->frequencies[EOF_SYMBOL] = 1;
    order0_context->total_freq = ALPHABET_SIZE + 1;

    // Reset history
    reset();

    // Pass 1: Count frequencies in all contexts
    for (size_t i = 0; i < data.size(); i++) {
        uint8_t byte = data[i];

        // Order-2 removed - no benefit over Order-1

        // Update Order-1 context
        if (history.size() >= 1) {
            ensure_context_exists(1);
            ContextModel::Context* ctx = get_context(1);
            if (ctx) {
                if (ctx->frequencies.find(byte) == ctx->frequencies.end()) {
                    ctx->frequencies[byte] = 0;
                }
                ctx->frequencies[byte]++;
                ctx->total_freq++;
            }
        }

        // Update Order-0 context (already initialized)
        if (order0_context->frequencies.find(byte) != order0_context->frequencies.end()) {
            order0_context->frequencies[byte]++;
            order0_context->total_freq++;
        }

        // Update history
        update_history(byte);
    }

    // Add escape symbols to all contexts that have seen symbols
    // Escape frequency based on context size (more symbols = higher escape probability)

    // Order-2 removed - no benefit over Order-1

    // Order-1 contexts
    for (auto& ctx_ptr : order1_contexts) {
        if (ctx_ptr && ctx_ptr->frequencies.size() > 0) {
            int unique_symbols = ctx_ptr->frequencies.size();
            // Escape frequency = unique_symbols / 4, minimum 1
            uint32_t escape_freq = std::max(1, unique_symbols / 4);
            ctx_ptr->frequencies[ESCAPE_SYMBOL] = escape_freq;
            ctx_ptr->total_freq += escape_freq;
            ctx_ptr->invalidate_cache();  // Mark cache as dirty
        }
    }

    // Order-0 context
    order0_context->invalidate_cache();

    // Reset history for encoding/decoding
    reset();

    std::cout << "Context model built successfully" << std::endl;
    print_statistics();
}

void ContextModel::update_history(uint8_t byte) {
    // Phase 2.2 Optimization: Use efficient history management
    if (history.size() >= MAX_ORDER) {
        // Rotate left: shift all elements left by 1, then set last element
        // This is O(n) but better than erase which reallocates
        // Alternative: std::rotate for cleaner code
        std::rotate(history.begin(), history.begin() + 1, history.end());
        history[MAX_ORDER - 1] = byte;
    } else {
        history.push_back(byte);
    }
}

void ContextModel::update_frequencies(uint8_t byte) {
    // In adaptive PPM, we update ALL context levels after encoding/decoding
    // This ensures both encoder and decoder build the same model

    // Determine which contexts we should update based on current history
    // NOTE: Order-2 disabled - no benefit over Order-1, adds complexity
    int max_order = (history.size() >= 1) ? 1 : 0;  // Was: >= 2 ? 2 : ...

    // Update from highest order down to Order-0
    for (int order = max_order; order >= 0; order--) {
        if (order == 0) {
            // Order-0: Always exists and always update
            order0_context->frequencies[byte]++;
            order0_context->total_freq++;
            order0_context->invalidate_cache();  // Mark cache as dirty
        } else {
            // Higher orders: ensure context exists, then update
            ensure_context_exists(order);
            ContextModel::Context* ctx = get_context(order);

            if (ctx) {
                // If symbol not in context, add it
                if (ctx->frequencies.find(byte) == ctx->frequencies.end()) {
                    ctx->frequencies[byte] = 1;  // Start with 1
                    ctx->total_freq += 1;

                    // Ensure escape exists, and update its frequency adaptively
                    if (ctx->frequencies.find(ESCAPE_SYMBOL) == ctx->frequencies.end()) {
                        // First time creating escape: set to 1
                        ctx->frequencies[ESCAPE_SYMBOL] = 1;
                        ctx->total_freq += 1;
                    } else {
                        // Update escape frequency based on context diversity
                        // More unique symbols = higher escape probability
                        int unique_symbols = ctx->frequencies.size() - 1;  // -1 for escape itself
                        uint32_t new_escape_freq = std::max(1, unique_symbols / 4);
                        uint32_t old_escape_freq = ctx->frequencies[ESCAPE_SYMBOL];

                        // Adjust total and escape frequency
                        ctx->total_freq = ctx->total_freq - old_escape_freq + new_escape_freq;
                        ctx->frequencies[ESCAPE_SYMBOL] = new_escape_freq;
                    }
                } else {
                    // Symbol already exists, just increment
                    ctx->frequencies[byte]++;
                    ctx->total_freq++;
                }

                ctx->invalidate_cache();  // Mark cache as dirty after any update
            }
        }
    }
}

// SRP: Main encoding dispatcher (DIP: depends on abstract strategy)
std::vector<ContextModel::EncodingStep> ContextModel::encode_symbol(uint8_t byte) {
    if (encoding_method_ == EncodingMethod::SIMPLE) {
        return encode_symbol_simple(byte);
    } else {
        return encode_symbol_with_exclusions(byte);
    }
}

// NEW: Simplified encoding without exclusions (SRP: single responsibility)
std::vector<ContextModel::EncodingStep> ContextModel::encode_symbol_simple(uint8_t byte) {
    std::vector<EncodingStep> steps;

    // Determine maximum order based on history
    int current_order = std::min(static_cast<int>(history.size()), MAX_ORDER);

    // For Phase 1, only use Order-1 (skip Order-2 for simplicity and speed)
    if (current_order > 1) {
        current_order = 1;
    }

    // Try encoding from highest to lowest order
    while (current_order >= 0) {
        Context* ctx = get_context(current_order);

        // Check if context exists and has the symbol
        if (ctx && ctx->has_symbol(byte)) {
            // Symbol found - encode it directly (no exclusions!)
            EncodingStep step;
            step.symbol = byte;
            get_symbol_range(current_order, byte,
                           step.cum_freq_low, step.cum_freq_high, step.total_freq);
            steps.push_back(step);
            return steps;  // Done!
        }

        // Symbol not found in this context
        if (current_order > 0 && ctx) {
            // Encode escape symbol to fall back to lower order
            EncodingStep escape_step;
            escape_step.symbol = ESCAPE_SYMBOL;
            get_symbol_range(current_order, ESCAPE_SYMBOL,
                           escape_step.cum_freq_low, escape_step.cum_freq_high,
                           escape_step.total_freq);
            steps.push_back(escape_step);
        }

        // Move to lower order
        current_order--;
    }

    // If we reach here, encode from Order-0 (guaranteed to have all symbols)
    EncodingStep step;
    step.symbol = byte;
    get_symbol_range(0, byte,
                   step.cum_freq_low, step.cum_freq_high, step.total_freq);
    steps.push_back(step);

    return steps;
}

std::vector<ContextModel::EncodingStep> ContextModel::encode_symbol_with_exclusions(uint8_t byte) {
    std::vector<EncodingStep> steps;
    std::vector<int> excluded_symbols;  // PPM Method C: track exclusions

    // Try Order-2 first
    int current_order = (history.size() >= 2) ? 2 : (history.size() >= 1) ? 1 : 0;

    while (current_order >= 0) {
        ContextModel::Context* ctx = get_context(current_order);

        if (ctx && ctx->has_symbol(byte)) {
            // Symbol found in this context, encode it (with exclusions from higher orders)
            EncodingStep step;
            step.symbol = byte;

            if (excluded_symbols.empty()) {
                // No exclusions, use normal encoding
                get_symbol_range(current_order, byte,
                               step.cum_freq_low, step.cum_freq_high, step.total_freq);
            } else {
                // Use exclusion-based encoding (PPM Method C)
                get_symbol_range_with_exclusions(current_order, byte, excluded_symbols,
                                               step.cum_freq_low, step.cum_freq_high, step.total_freq);
            }

            steps.push_back(step);
            break;
        } else if (ctx && current_order > 0) {
            // Symbol not found at this order
            // Encode escape (NOTE: escape symbol itself is NOT affected by exclusions)
            EncodingStep escape_step;
            escape_step.symbol = ESCAPE_SYMBOL;
            get_symbol_range(current_order, ESCAPE_SYMBOL,
                           escape_step.cum_freq_low, escape_step.cum_freq_high,
                           escape_step.total_freq);
            steps.push_back(escape_step);

            // PPM Method C: After escaping, add all symbols seen at this order to exclusion list
            for (const auto& pair : ctx->frequencies) {
                if (pair.first != ESCAPE_SYMBOL && pair.first != EOF_SYMBOL) {
                    excluded_symbols.push_back(pair.first);
                }
            }

            current_order--;
        } else {
            // Order-0: symbol must exist (all symbols initialized)
            EncodingStep step;
            step.symbol = byte;

            if (excluded_symbols.empty()) {
                get_symbol_range(0, byte,
                               step.cum_freq_low, step.cum_freq_high, step.total_freq);
            } else {
                get_symbol_range_with_exclusions(0, byte, excluded_symbols,
                                               step.cum_freq_low, step.cum_freq_high, step.total_freq);
            }

            steps.push_back(step);
            break;
        }
    }

    return steps;
}

void ContextModel::get_symbol_range(int order, int symbol,
                                   uint32_t& cum_freq_low,
                                   uint32_t& cum_freq_high,
                                   uint32_t& total_freq) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);

    if (!ctx || ctx->frequencies.empty()) {
        throw std::runtime_error("Context not found or empty");
    }

    // Ensure cache is valid (rebuilds only if dirty)
    ctx->ensure_cache_valid();

    // Binary search for the symbol in sorted cached_symbols
    auto it = std::lower_bound(ctx->cached_symbols.begin(), ctx->cached_symbols.end(), symbol);
    if (it == ctx->cached_symbols.end() || *it != symbol) {
        throw std::runtime_error("Symbol not found in context");
    }

    size_t idx = std::distance(ctx->cached_symbols.begin(), it);
    cum_freq_low = ctx->cached_cumulative[idx];
    cum_freq_high = (idx + 1 < ctx->cached_cumulative.size()) ?
                    ctx->cached_cumulative[idx + 1] : ctx->total_freq;
    total_freq = ctx->total_freq;
}

void ContextModel::Context::ensure_cache_valid() {
    if (!cumulative_dirty) {
        return;  // Cache is still valid
    }

    // Rebuild cache
    cached_symbols.clear();
    cached_cumulative.clear();

    // Sort symbols by their numeric value for consistency
    for (const auto& pair : frequencies) {
        cached_symbols.push_back(pair.first);
    }
    std::sort(cached_symbols.begin(), cached_symbols.end());

    // Build cumulative frequencies
    uint32_t cum = 0;
    for (int symbol : cached_symbols) {
        cached_cumulative.push_back(cum);
        cum += frequencies.at(symbol);
    }

    cumulative_dirty = false;  // Mark cache as clean
}

void ContextModel::build_cumulative_freq(const ContextModel::Context* ctx,
                                        std::vector<int>& symbols,
                                        std::vector<uint32_t>& cumulative) const {
    // This method is now deprecated but kept for compatibility
    // It just uses the cached data
    ContextModel::Context* mutable_ctx = const_cast<ContextModel::Context*>(ctx);
    mutable_ctx->ensure_cache_valid();
    symbols = ctx->cached_symbols;
    cumulative = ctx->cached_cumulative;
}

int ContextModel::find_symbol(int order, uint32_t cum_freq) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);

    if (!ctx || ctx->frequencies.empty()) {
        return -1;
    }

    // Ensure cache is valid (rebuilds only if dirty)
    ctx->ensure_cache_valid();

    // Binary search for the cumulative frequency range
    // Find the largest cumulative value <= cum_freq
    auto it = std::upper_bound(ctx->cached_cumulative.begin(), ctx->cached_cumulative.end(), cum_freq);

    if (it == ctx->cached_cumulative.begin()) {
        return -1;  // cum_freq is before first symbol (shouldn't happen)
    }

    --it;  // Move back to the actual symbol
    size_t idx = std::distance(ctx->cached_cumulative.begin(), it);

    // Verify the range
    uint32_t low = ctx->cached_cumulative[idx];
    uint32_t high = (idx + 1 < ctx->cached_cumulative.size()) ?
                    ctx->cached_cumulative[idx + 1] : ctx->total_freq;

    if (cum_freq >= low && cum_freq < high) {
        return ctx->cached_symbols[idx];
    }

    return -1;  // Not found (should not happen)
}

bool ContextModel::has_symbol_in_context(int order, int symbol) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);
    if (!ctx) return false;
    return ctx->has_symbol(symbol);
}

uint32_t ContextModel::get_total_freq(int order) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);
    if (!ctx) return 0;
    return ctx->total_freq;
}

void ContextModel::print_statistics() const {
    std::cout << "=== Context Model Statistics ===" << std::endl;

    // Count non-empty contexts
    int order1_count = 0;
    for (const auto& ctx : order1_contexts) {
        if (ctx && ctx->frequencies.size() > 0) order1_count++;
    }

    // Order-2 removed - no benefit over Order-1
    std::cout << "Order-1 contexts: " << order1_count << " / 256" << std::endl;
    std::cout << "Order-0 symbols: " << order0_context->frequencies.size() << std::endl;
}

// PPM Method C: Get symbol range with exclusions
void ContextModel::get_symbol_range_with_exclusions(int order, int symbol,
                                                   const std::vector<int>& excluded_symbols,
                                                   uint32_t& cum_freq_low,
                                                   uint32_t& cum_freq_high,
                                                   uint32_t& total_freq) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);

    if (!ctx || ctx->frequencies.empty()) {
        throw std::runtime_error("Context not found or empty in get_symbol_range_with_exclusions");
    }

    // Build temporary frequency distribution excluding specified symbols
    std::vector<int> valid_symbols;
    std::vector<uint32_t> valid_cumulative;

    uint32_t cum = 0;
    total_freq = 0;

    // First, calculate total frequency excluding excluded symbols
    for (const auto& pair : ctx->frequencies) {
        bool is_excluded = false;
        for (int excluded : excluded_symbols) {
            if (pair.first == excluded) {
                is_excluded = true;
                break;
            }
        }

        if (!is_excluded) {
            total_freq += pair.second;
        }
    }

    if (total_freq == 0) {
        throw std::runtime_error("All symbols excluded in get_symbol_range_with_exclusions");
    }

    // Build cumulative frequencies for non-excluded symbols (sorted order)
    std::vector<std::pair<int, uint32_t>> sorted_freqs;
    for (const auto& pair : ctx->frequencies) {
        bool is_excluded = false;
        for (int excluded : excluded_symbols) {
            if (pair.first == excluded) {
                is_excluded = true;
                break;
            }
        }

        if (!is_excluded) {
            sorted_freqs.push_back({pair.first, pair.second});
        }
    }

    // Sort by symbol value for consistency
    std::sort(sorted_freqs.begin(), sorted_freqs.end(),
             [](const auto& a, const auto& b) { return a.first < b.first; });

    // Build cumulative frequency array
    for (const auto& pair : sorted_freqs) {
        valid_symbols.push_back(pair.first);
        valid_cumulative.push_back(cum);
        cum += pair.second;
    }

    // Find the requested symbol
    auto it = std::find(valid_symbols.begin(), valid_symbols.end(), symbol);
    if (it == valid_symbols.end()) {
        throw std::runtime_error("Symbol not found in excluded distribution");
    }

    size_t idx = std::distance(valid_symbols.begin(), it);
    cum_freq_low = valid_cumulative[idx];
    cum_freq_high = (idx + 1 < valid_cumulative.size()) ?
                    valid_cumulative[idx + 1] : total_freq;
}

// PPM Method C: Find symbol with exclusions
int ContextModel::find_symbol_with_exclusions(int order, uint32_t cum_freq,
                                             const std::vector<int>& excluded_symbols) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);

    if (!ctx || ctx->frequencies.empty()) {
        return -1;
    }

    // Build temporary frequency distribution excluding specified symbols
    std::vector<int> valid_symbols;
    std::vector<uint32_t> valid_cumulative;

    uint32_t cum = 0;
    uint32_t total_freq = 0;

    // Calculate total frequency excluding excluded symbols
    for (const auto& pair : ctx->frequencies) {
        bool is_excluded = false;
        for (int excluded : excluded_symbols) {
            if (pair.first == excluded) {
                is_excluded = true;
                break;
            }
        }

        if (!is_excluded) {
            total_freq += pair.second;
        }
    }

    if (total_freq == 0) {
        return -1;
    }

    // Build cumulative frequencies for non-excluded symbols (sorted order)
    std::vector<std::pair<int, uint32_t>> sorted_freqs;
    for (const auto& pair : ctx->frequencies) {
        bool is_excluded = false;
        for (int excluded : excluded_symbols) {
            if (pair.first == excluded) {
                is_excluded = true;
                break;
            }
        }

        if (!is_excluded) {
            sorted_freqs.push_back({pair.first, pair.second});
        }
    }

    // Sort by symbol value for consistency
    std::sort(sorted_freqs.begin(), sorted_freqs.end(),
             [](const auto& a, const auto& b) { return a.first < b.first; });

    // Build cumulative frequency array
    for (const auto& pair : sorted_freqs) {
        valid_symbols.push_back(pair.first);
        valid_cumulative.push_back(cum);
        cum += pair.second;
    }

    // Binary search for the cumulative frequency range
    auto it = std::upper_bound(valid_cumulative.begin(), valid_cumulative.end(), cum_freq);

    if (it == valid_cumulative.begin()) {
        return -1;  // cum_freq is before first symbol
    }

    --it;  // Move back to the actual symbol
    size_t idx = std::distance(valid_cumulative.begin(), it);

    // Verify the range
    uint32_t low = valid_cumulative[idx];
    uint32_t high = (idx + 1 < valid_cumulative.size()) ?
                    valid_cumulative[idx + 1] : total_freq;

    if (cum_freq >= low && cum_freq < high) {
        return valid_symbols[idx];
    }

    return -1;  // Not found
}

// Helper: Get all symbols in a context (for PPM exclusions)
std::vector<int> ContextModel::get_context_symbols(int order) const {
    std::vector<int> symbols;

    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);
    if (!ctx) {
        return symbols;  // Empty if context doesn't exist
    }

    // Return all symbols except escape and EOF
    for (const auto& pair : ctx->frequencies) {
        if (pair.first != ESCAPE_SYMBOL && pair.first != EOF_SYMBOL) {
            symbols.push_back(pair.first);
        }
    }

    return symbols;
}

// Get total frequency with exclusions
uint32_t ContextModel::get_total_freq_with_exclusions(int order, const std::vector<int>& excluded_symbols) const {
    ContextModel::Context* ctx = const_cast<ContextModel*>(this)->get_context(order);
    if (!ctx) return 0;

    uint32_t total = 0;
    for (const auto& pair : ctx->frequencies) {
        bool is_excluded = false;
        for (int excluded : excluded_symbols) {
            if (pair.first == excluded) {
                is_excluded = true;
                break;
            }
        }

        if (!is_excluded) {
            total += pair.second;
        }
    }

    return total;
}

