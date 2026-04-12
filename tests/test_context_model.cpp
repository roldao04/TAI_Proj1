#include "arithmetic/range_coder.h"
#include "model/context_model.h"

#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

namespace {

constexpr uint8_t O2_HIT_SYMBOL = 'x';
constexpr uint8_t O1_ONLY_SYMBOL = 'y';
constexpr uint8_t O0_ONLY_SYMBOL = 'z';

void train_model(ContextModel& model, const std::string& data) {
    for (char ch : data) {
        const uint8_t byte = static_cast<uint8_t>(ch);
        model.update_frequencies(byte);
        model.update_history(byte);
    }
}

ContextModel build_ready_model() {
    ContextModel model;
    model.set_encoding_method_ppm_c();
    model.enable_order2();
    model.init_adaptive();

    std::string training;
    for (int i = 0; i < 24; ++i) {
        training += "abxcbyaz";
    }
    training += "ab";

    train_model(model, training);

    assert(model.has_order2_context());
    assert(model.has_order1_context());
    return model;
}

void assert_steps_match(const std::vector<ContextModel::EncodingStep>& slow,
                        const ContextModel::EncodeResult& fast,
                        const std::vector<int>& expected_symbols,
                        const std::string& label) {
    assert(slow.size() == expected_symbols.size());
    assert(static_cast<size_t>(fast.count) == expected_symbols.size());

    for (size_t i = 0; i < expected_symbols.size(); ++i) {
        assert(slow[i].symbol == expected_symbols[i]);
        assert(fast.steps[i].symbol == expected_symbols[i]);
        assert(slow[i].cum_freq_low == fast.steps[i].cum_freq_low);
        assert(slow[i].cum_freq_high == fast.steps[i].cum_freq_high);
        assert(slow[i].total_freq == fast.steps[i].total_freq);
    }

    std::cout << "  " << label << " OK" << std::endl;
}

uint8_t decode_single_symbol(const std::vector<uint8_t>& encoded) {
    ContextModel model = build_ready_model();
    RangeDecoder decoder(encoded);

    uint32_t lo = 0;
    uint32_t hi = 0;
    uint32_t total = 0;

    uint32_t cum = decoder.get_current_count(model.get_order2_total());
    int sym = model.find_symbol_and_get_range_o2(cum, lo, hi, total);
    assert(sym >= 0);
    decoder.decode_symbol(lo, hi, total);

    if (sym == ContextModel::ESCAPE_SYMBOL) {
        const uint32_t* o2_freq = model.get_order2_freq_ptr();
        const uint32_t o1_total = model.get_order1_total_excl_ctx(o2_freq);
        cum = decoder.get_current_count(o1_total);
        sym = model.find_symbol_and_get_range_order1_excl_ctx(cum, o2_freq, o1_total, lo, hi, total);
        assert(sym >= 0);
        decoder.decode_symbol(lo, hi, total);

        if (sym == ContextModel::ESCAPE_SYMBOL) {
            const uint32_t* o1_freq = model.get_order1_freq_ptr();
            const uint32_t o0_total = model.get_order0_total_excl_ctx(o1_freq);
            cum = decoder.get_current_count(o0_total);
            sym = model.find_symbol_and_get_range_excl_ctx(cum, o1_freq, o0_total, lo, hi, total);
            assert(sym >= 0);
            decoder.decode_symbol(lo, hi, total);
        }
    }

    return static_cast<uint8_t>(sym);
}

void assert_path_and_decode(uint8_t symbol,
                            const std::vector<int>& expected_symbols,
                            const std::string& label) {
    ContextModel model = build_ready_model();

    const auto slow = model.encode_symbol(symbol);
    const auto fast = model.encode_symbol_fast(symbol);
    assert_steps_match(slow, fast, expected_symbols, label);

    RangeEncoder encoder;
    for (const auto& step : slow) {
        encoder.encode_symbol(step.cum_freq_low, step.cum_freq_high, step.total_freq);
    }
    encoder.finish();

    const uint8_t decoded = decode_single_symbol(encoder.get_output());
    assert(decoded == symbol);
}

void test_order2_direct_hit() {
    std::cout << "Test: direct O2 hit" << std::endl;
    assert_path_and_decode(O2_HIT_SYMBOL, {O2_HIT_SYMBOL}, "O2 direct hit");
}

void test_order2_escape_to_order1_direct_with_exclusions() {
    std::cout << "Test: O2 escape to O1 direct with exclusions" << std::endl;
    assert_path_and_decode(O1_ONLY_SYMBOL,
                           {ContextModel::ESCAPE_SYMBOL, O1_ONLY_SYMBOL},
                           "O2 escape then O1 direct");
}

void test_order2_escape_to_order1_escape_to_order0() {
    std::cout << "Test: O2 escape to O1 escape to O0" << std::endl;
    assert_path_and_decode(O0_ONLY_SYMBOL,
                           {ContextModel::ESCAPE_SYMBOL, ContextModel::ESCAPE_SYMBOL, O0_ONLY_SYMBOL},
                           "O2 then O1 escape to O0");
}

void test_fast_non_fast_consistency_with_order2() {
    std::cout << "Test: fast/non-fast consistency with O2 enabled" << std::endl;
    assert_path_and_decode(O2_HIT_SYMBOL, {O2_HIT_SYMBOL}, "fast vs slow O2 hit");
    assert_path_and_decode(O1_ONLY_SYMBOL,
                           {ContextModel::ESCAPE_SYMBOL, O1_ONLY_SYMBOL},
                           "fast vs slow O2->O1");
    assert_path_and_decode(O0_ONLY_SYMBOL,
                           {ContextModel::ESCAPE_SYMBOL, ContextModel::ESCAPE_SYMBOL, O0_ONLY_SYMBOL},
                           "fast vs slow O2->O1->O0");
}

} // namespace

int main() {
    test_order2_direct_hit();
    test_order2_escape_to_order1_direct_with_exclusions();
    test_order2_escape_to_order1_escape_to_order0();
    test_fast_non_fast_consistency_with_order2();
    std::cout << "All ContextModel tests passed" << std::endl;
    return 0;
}
