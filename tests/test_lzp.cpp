#include <cassert>
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>
#include "transform/bwt.h"
#include "transform/lzp.h"
#include "transform/mtf.h"
#include "transform/zero_rle.h"

std::vector<uint8_t> str_to_vec(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

std::vector<uint8_t> make_deterministic_noise(size_t size) {
    std::vector<uint8_t> data(size);
    uint32_t state = 0x12345678u;
    for (size_t i = 0; i < size; i++) {
        state = state * 1664525u + 1013904223u;
        data[i] = static_cast<uint8_t>(state >> 24);
    }
    return data;
}

void test_lzp_round_trip_on_compressible_data() {
    std::cout << "Test 1: LZP round trip on compressible input..." << std::endl;

    std::string chunk = "banana bandana banana bandana abracadabra abracadabra ";
    std::string input;
    for (int i = 0; i < 128; i++) {
        input += chunk;
    }

    std::vector<uint8_t> original = str_to_vec(input);
    std::vector<uint8_t> encoded = LZPredictor::encode(original);

    assert(!encoded.empty());
    assert(encoded.size() < original.size());

    std::vector<uint8_t> decoded = LZPredictor::decode(encoded, original.size());
    assert(decoded == original);

    std::cout << "  Original size: " << original.size() << " bytes" << std::endl;
    std::cout << "  LZP size: " << encoded.size() << " bytes" << std::endl;
    std::cout << "  ✓ Passed\n" << std::endl;
}

void test_lzp_skip_on_incompressible_data() {
    std::cout << "Test 2: LZP skips incompressible input..." << std::endl;

    std::vector<uint8_t> input = make_deterministic_noise(4096);
    std::vector<uint8_t> encoded = LZPredictor::encode(input);

    assert(encoded.empty());

    std::cout << "  Input size: " << input.size() << " bytes" << std::endl;
    std::cout << "  ✓ Passed\n" << std::endl;
}

void test_lzp_bwt_pipeline_round_trip() {
    std::cout << "Test 3: LZP + BWT + MTF + ZRLE round trip..." << std::endl;

    std::string chunk = "TOBEORNOTTOBEORTOBEORNOT#";
    std::string input;
    for (int i = 0; i < 160; i++) {
        input += chunk;
    }

    std::vector<uint8_t> original = str_to_vec(input);
    std::vector<uint8_t> lzp = LZPredictor::encode(original);
    assert(!lzp.empty());

    auto [bwt, primary_index] = BWT::transform(lzp);
    std::vector<uint8_t> mtf = MoveToFront::transform(bwt);
    std::vector<uint8_t> zrle = ZeroRunLengthEncoder::encode(mtf);

    std::vector<uint8_t> reversed = ZeroRunLengthEncoder::decode(zrle);
    reversed = MoveToFront::inverse_transform(reversed);
    reversed = BWT::inverse_transform(reversed, primary_index);
    reversed = LZPredictor::decode(reversed, original.size());

    assert(reversed == original);

    std::cout << "  ✓ Passed\n" << std::endl;
}

int main() {
    std::cout << "=== LZP Tests ===\n" << std::endl;

    test_lzp_round_trip_on_compressible_data();
    test_lzp_skip_on_incompressible_data();
    test_lzp_bwt_pipeline_round_trip();

    std::cout << "All LZP tests passed." << std::endl;
    return 0;
}
