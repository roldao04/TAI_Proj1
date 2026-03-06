#include <iostream>
#include <string>
#include <vector>
#include <cassert>
#include "transform/bwt.h"

// Helper to convert string to vector<uint8_t>
std::vector<uint8_t> str_to_vec(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

// Helper to convert vector<uint8_t> to string
std::string vec_to_str(const std::vector<uint8_t>& v) {
    return std::string(v.begin(), v.end());
}

void test_basic_transform() {
    std::cout << "Test 1: Basic Transform..." << std::endl;

    std::string input = "banana";
    auto input_vec = str_to_vec(input);

    auto [bwt_output, primary_index] = BWT::transform(input_vec);
    std::string bwt_str = vec_to_str(bwt_output);

    std::cout << "  Input: " << input << std::endl;
    std::cout << "  BWT output: " << bwt_str << std::endl;
    std::cout << "  Primary index: " << primary_index << std::endl;

    // The exact BWT output depends on how we handle the sentinel
    // But it should be a permutation of input characters
    assert(bwt_output.size() == input_vec.size());
    std::cout << "  ✓ Passed\n" << std::endl;
}

void test_invertibility() {
    std::cout << "Test 2: Invertibility..." << std::endl;

    std::vector<std::string> test_strings = {
        "banana",
        "mississippi",
        "abracadabra",
        "hello world",
        "aaaaaaaaaa",
        "abcdefghij",
        "The quick brown fox jumps over the lazy dog"
    };

    for (const auto& test_str : test_strings) {
        auto input_vec = str_to_vec(test_str);

        // Forward transform
        auto [bwt_output, primary_index] = BWT::transform(input_vec);

        // Inverse transform
        auto reconstructed = BWT::inverse_transform(bwt_output, primary_index);

        // Verify
        if (reconstructed != input_vec) {
            std::cerr << "  ✗ FAILED on: " << test_str << std::endl;
            std::cerr << "    Expected: " << test_str << std::endl;
            std::cerr << "    Got: " << vec_to_str(reconstructed) << std::endl;
            assert(false);
        }

        std::cout << "  ✓ \"" << test_str << "\" → BWT → inverse = original" << std::endl;
    }

    std::cout << "  ✓ All invertibility tests passed\n" << std::endl;
}

void test_block_processing() {
    std::cout << "Test 3: Block Processing..." << std::endl;

    // Simple test first: 2 blocks of known data
    std::string block1_str = "aaaabbbbccccdddd";  // 16 bytes
    std::string block2_str = "11112222333344";    // 14 bytes
    std::string simple_input = block1_str + block2_str;  // 30 bytes total

    auto input_vec = str_to_vec(simple_input);
    std::cout << "  Simple test - Input size: " << input_vec.size() << " bytes" << std::endl;

    // Transform with 16-byte blocks (so we get 2 blocks: 16 + 14)
    size_t small_block_size = 16;
    auto [bwt_output, primary_indices] = BWT::transform_blocks(input_vec, small_block_size);

    std::cout << "  Number of blocks: " << primary_indices.size() << std::endl;
    std::cout << "  BWT output size: " << bwt_output.size() << " bytes" << std::endl;
    std::cout << "  Primary indices: ";
    for (auto idx : primary_indices) {
        std::cout << idx << " ";
    }
    std::cout << std::endl;

    // Manually test each block
    std::cout << "  Testing block 1 (16 bytes)..." << std::endl;
    auto block1_vec = str_to_vec(block1_str);
    auto [block1_bwt, block1_idx] = BWT::transform(block1_vec);
    auto block1_reconstructed = BWT::inverse_transform(block1_bwt, block1_idx);
    if (block1_reconstructed != block1_vec) {
        std::cerr << "    ✗ Block 1 alone failed!" << std::endl;
        assert(false);
    }
    std::cout << "    ✓ Block 1 alone works" << std::endl;

    std::cout << "  Testing block 2 (14 bytes)..." << std::endl;
    auto block2_vec = str_to_vec(block2_str);
    auto [block2_bwt, block2_idx] = BWT::transform(block2_vec);
    auto block2_reconstructed = BWT::inverse_transform(block2_bwt, block2_idx);
    if (block2_reconstructed != block2_vec) {
        std::cerr << "    ✗ Block 2 alone failed!" << std::endl;
        assert(false);
    }
    std::cout << "    ✓ Block 2 alone works" << std::endl;

    // Inverse transform
    auto reconstructed = BWT::inverse_transform_blocks(bwt_output, primary_indices, small_block_size);

    // Verify
    std::cout << "  Reconstructed size: " << reconstructed.size() << " bytes" << std::endl;

    if (reconstructed.size() != input_vec.size()) {
        std::cerr << "  ✗ FAILED: Size mismatch!" << std::endl;
        std::cerr << "    Expected: " << input_vec.size() << " bytes" << std::endl;
        std::cerr << "    Got: " << reconstructed.size() << " bytes" << std::endl;
        assert(false);
    }

    if (reconstructed != input_vec) {
        std::cerr << "  ✗ FAILED: Content mismatch!" << std::endl;
        std::cerr << "    Expected: " << simple_input << std::endl;
        std::cerr << "    Got:      " << vec_to_str(reconstructed) << std::endl;
        // Find first difference
        for (size_t i = 0; i < std::min(reconstructed.size(), input_vec.size()); i++) {
            if (reconstructed[i] != input_vec[i]) {
                std::cerr << "    First difference at byte " << i << std::endl;
                std::cerr << "    Expected char: '" << (char)input_vec[i] << "' (0x" << std::hex << (int)input_vec[i] << ")" << std::endl;
                std::cerr << "    Got char:      '" << (char)reconstructed[i] << "' (0x" << (int)reconstructed[i] << ")" << std::dec << std::endl;
                break;
            }
        }
        assert(false);
    }

    std::cout << "  ✓ Simple block processing works!" << std::endl;

    // Now test larger input
    std::cout << "\n  Testing larger input..." << std::endl;
    std::string large_input = "";
    for (int i = 0; i < 100; i++) {
        large_input += "The quick brown fox jumps over the lazy dog. ";
    }

    auto large_vec = str_to_vec(large_input);
    std::cout << "  Large input size: " << large_vec.size() << " bytes" << std::endl;

    auto [large_bwt, large_indices] = BWT::transform_blocks(large_vec, 1024);
    std::cout << "  Number of blocks: " << large_indices.size() << std::endl;
    std::cout << "  BWT output size: " << large_bwt.size() << " bytes" << std::endl;
    std::cout << "  Primary indices: ";
    for (size_t i = 0; i < std::min(large_indices.size(), size_t(10)); i++) {
        std::cout << large_indices[i] << " ";
    }
    if (large_indices.size() > 10) {
        std::cout << "... (" << large_indices.size() << " total)";
    }
    std::cout << std::endl;

    // Validate: manually transform/inverse first block to check
    std::cout << "  Validating first block manually..." << std::endl;
    size_t first_block_size = std::min(size_t(1024), large_vec.size());
    std::vector<uint8_t> first_block_input(large_vec.begin(), large_vec.begin() + first_block_size);
    auto [first_block_bwt, first_block_idx] = BWT::transform(first_block_input);

    std::cout << "    Manual transform: primary_index = " << first_block_idx << std::endl;
    std::cout << "    From transform_blocks: primary_index = " << large_indices[0] << std::endl;

    if (first_block_idx != large_indices[0]) {
        std::cerr << "    ✗ WARNING: Primary index mismatch for block 0!" << std::endl;
    }

    // Check if BWT outputs match
    std::vector<uint8_t> first_block_bwt_from_blocks(large_bwt.begin(), large_bwt.begin() + first_block_size);
    if (first_block_bwt != first_block_bwt_from_blocks) {
        std::cerr << "    ✗ WARNING: BWT output mismatch for block 0!" << std::endl;
    }

    // Try inverse with the manual index
    auto first_block_reconstructed = BWT::inverse_transform(first_block_bwt, first_block_idx);
    if (first_block_reconstructed == first_block_input) {
        std::cout << "    ✓ Manual transform/inverse of block 0 works" << std::endl;
    } else {
        std::cerr << "    ✗ Manual transform/inverse of block 0 FAILED" << std::endl;
    }

    auto large_reconstructed = BWT::inverse_transform_blocks(large_bwt, large_indices, 1024);

    std::cout << "  Reconstructed size: " << large_reconstructed.size() << " bytes" << std::endl;

    if (large_reconstructed.size() != large_vec.size()) {
        std::cerr << "  ✗ FAILED: Size mismatch!" << std::endl;
        std::cerr << "    Expected: " << large_vec.size() << " bytes" << std::endl;
        std::cerr << "    Got: " << large_reconstructed.size() << " bytes" << std::endl;
        assert(false);
    }

    if (large_reconstructed != large_vec) {
        std::cerr << "  ✗ FAILED: Large input content mismatch" << std::endl;
        // Find first difference
        for (size_t i = 0; i < std::min(large_reconstructed.size(), large_vec.size()); i++) {
            if (large_reconstructed[i] != large_vec[i]) {
                std::cerr << "    First difference at byte " << i << std::endl;
                std::cerr << "    Expected: '" << (char)large_vec[i] << "' (0x" << std::hex << (int)large_vec[i] << ")" << std::endl;
                std::cerr << "    Got:      '" << (char)large_reconstructed[i] << "' (0x" << (int)large_reconstructed[i] << ")" << std::dec << std::endl;

                // Show context
                size_t ctx_start = (i >= 20) ? i - 20 : 0;
                size_t ctx_end = std::min(i + 20, large_vec.size());
                std::cerr << "    Context (expected): \"";
                for (size_t j = ctx_start; j < ctx_end; j++) {
                    char c = large_vec[j];
                    if (c >= 32 && c < 127) std::cerr << c;
                    else std::cerr << '.';
                }
                std::cerr << "\"" << std::endl;
                std::cerr << "    Context (got):      \"";
                for (size_t j = ctx_start; j < ctx_end; j++) {
                    if (j < large_reconstructed.size()) {
                        char c = large_reconstructed[j];
                        if (c >= 32 && c < 127) std::cerr << c;
                        else std::cerr << '.';
                    }
                }
                std::cerr << "\"" << std::endl;
                break;
            }
        }
        assert(false);
    }

    std::cout << "  ✓ Large input works too!" << std::endl;
    std::cout << "  ✓ All block processing tests passed\n" << std::endl;
}

void test_edge_cases() {
    std::cout << "Test 4: Edge Cases..." << std::endl;

    // Empty string
    {
        std::vector<uint8_t> empty;
        auto [bwt_output, primary_index] = BWT::transform(empty);
        assert(bwt_output.empty());
        std::cout << "  ✓ Empty string handled" << std::endl;
    }

    // Single character
    {
        auto input = str_to_vec("a");
        auto [bwt_output, primary_index] = BWT::transform(input);
        auto reconstructed = BWT::inverse_transform(bwt_output, primary_index);
        assert(reconstructed == input);
        std::cout << "  ✓ Single character handled" << std::endl;
    }

    // All same characters
    {
        auto input = str_to_vec("aaaaaaaaaa");
        auto [bwt_output, primary_index] = BWT::transform(input);
        auto reconstructed = BWT::inverse_transform(bwt_output, primary_index);
        assert(reconstructed == input);
        std::cout << "  ✓ All same characters handled" << std::endl;
    }

    // Binary data
    {
        std::vector<uint8_t> binary_data;
        for (int i = 0; i < 256; i++) {
            binary_data.push_back(static_cast<uint8_t>(i));
        }
        auto [bwt_output, primary_index] = BWT::transform(binary_data);
        auto reconstructed = BWT::inverse_transform(bwt_output, primary_index);
        assert(reconstructed == binary_data);
        std::cout << "  ✓ Binary data handled" << std::endl;
    }

    std::cout << "  ✓ All edge cases passed\n" << std::endl;
}

int main() {
    std::cout << "=== BWT Unit Tests ===" << std::endl << std::endl;

    try {
        test_basic_transform();
        test_invertibility();
        test_block_processing();
        test_edge_cases();

        std::cout << "=== All Tests Passed! ===" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
