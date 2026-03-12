#include "transform/bwt.h"
#include "libsais.h"
#include <algorithm>
#include <stdexcept>
#include <thread>
#include <vector>

/*
 * BWT Implementation
 *
 * Uses O(n log n) prefix-doubling suffix array construction for cyclic
 * rotations. Block-based processing for large files (900KB blocks,
 * similar to bzip2).
 */

// ============================================================================
// Suffix Array Construction
// ============================================================================

std::vector<uint32_t> BWT::build_suffix_array_prefix_doubling(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return std::vector<uint32_t>();
    }

    uint32_t n = data.size();
    std::vector<uint32_t> suffix_array(n);
    std::vector<uint32_t> equivalence_class(n);
    std::vector<uint32_t> next_suffix_array(n);
    std::vector<uint32_t> next_equivalence_class(n);
    std::vector<uint32_t> count(std::max<size_t>(256, n), 0);

    for (uint32_t i = 0; i < n; i++) {
        count[data[i]]++;
    }

    for (size_t i = 1; i < 256; i++) {
        count[i] += count[i - 1];
    }

    for (int i = static_cast<int>(n) - 1; i >= 0; i--) {
        uint8_t byte = data[i];
        suffix_array[--count[byte]] = i;
    }

    equivalence_class[suffix_array[0]] = 0;
    uint32_t class_count = 1;
    for (uint32_t i = 1; i < n; i++) {
        if (data[suffix_array[i]] != data[suffix_array[i - 1]]) {
            class_count++;
        }
        equivalence_class[suffix_array[i]] = class_count - 1;
    }

    for (uint32_t shift = 1; shift < n; shift <<= 1) {
        for (uint32_t i = 0; i < n; i++) {
            next_suffix_array[i] = (suffix_array[i] + n - shift) % n;
        }

        std::fill(count.begin(), count.begin() + class_count, 0);
        for (uint32_t i = 0; i < n; i++) {
            count[equivalence_class[next_suffix_array[i]]]++;
        }
        for (uint32_t i = 1; i < class_count; i++) {
            count[i] += count[i - 1];
        }
        for (int i = static_cast<int>(n) - 1; i >= 0; i--) {
            uint32_t start = next_suffix_array[i];
            uint32_t current_class = equivalence_class[start];
            suffix_array[--count[current_class]] = start;
        }

        next_equivalence_class[suffix_array[0]] = 0;
        uint32_t next_class_count = 1;
        for (uint32_t i = 1; i < n; i++) {
            uint32_t current = suffix_array[i];
            uint32_t previous = suffix_array[i - 1];

            std::pair<uint32_t, uint32_t> current_pair = {
                equivalence_class[current],
                equivalence_class[(current + shift) % n]
            };
            std::pair<uint32_t, uint32_t> previous_pair = {
                equivalence_class[previous],
                equivalence_class[(previous + shift) % n]
            };

            if (current_pair != previous_pair) {
                next_class_count++;
            }
            next_equivalence_class[current] = next_class_count - 1;
        }

        equivalence_class.swap(next_equivalence_class);
        class_count = next_class_count;

        if (class_count == n) {
            break;
        }
    }

    return suffix_array;
}

// ============================================================================
// Forward BWT Transform
// ============================================================================

std::pair<std::vector<uint8_t>, uint32_t>
BWT::transform(const std::vector<uint8_t>& input) {
    if (input.empty()) {
        return {std::vector<uint8_t>(), 0};
    }

    int32_t n = static_cast<int32_t>(input.size());
    std::vector<uint8_t> bwt_output(n);
    // libsais requires a temporary working buffer of size n
    std::vector<int32_t> tmp(n);

    int32_t primary_index = libsais_bwt(
        input.data(), bwt_output.data(), tmp.data(), n, 0, nullptr);

    if (primary_index < 0) {
        throw std::runtime_error("libsais_bwt failed");
    }

    return {bwt_output, static_cast<uint32_t>(primary_index)};
}

// ============================================================================
// Inverse BWT Transform
// ============================================================================

std::vector<uint8_t>
BWT::inverse_transform(const std::vector<uint8_t>& bwt_data, uint32_t primary_index) {
    if (bwt_data.empty()) {
        return std::vector<uint8_t>();
    }

    int32_t n = static_cast<int32_t>(bwt_data.size());
    std::vector<uint8_t> output(n);
    std::vector<int32_t> tmp(n + 1);  // libsais requires n+1

    int32_t ret = libsais_unbwt(
        bwt_data.data(), output.data(), tmp.data(), n, nullptr,
        static_cast<int32_t>(primary_index));

    if (ret != 0) {
        throw std::runtime_error("libsais_unbwt failed");
    }

    return output;
}

// ============================================================================
// Block-based Processing
// ============================================================================

std::pair<std::vector<uint8_t>, std::vector<uint32_t>>
BWT::transform_blocks(const std::vector<uint8_t>& input, size_t block_size) {
    if (input.empty()) {
        return {std::vector<uint8_t>(), std::vector<uint32_t>()};
    }

    size_t num_blocks = (input.size() + block_size - 1) / block_size;

    // Pre-allocate per-block result storage so threads write independently
    struct BlockResult {
        std::vector<uint8_t> bwt_output;
        uint32_t primary_index = 0;
    };
    std::vector<BlockResult> results(num_blocks);

    // Launch one thread per block — blocks are fully independent
    std::vector<std::thread> threads;
    threads.reserve(num_blocks);

    for (size_t block_idx = 0; block_idx < num_blocks; block_idx++) {
        size_t start = block_idx * block_size;
        size_t end = std::min(start + block_size, input.size());

        threads.emplace_back([&input, &results, block_idx, start, end]() {
            std::vector<uint8_t> block(input.begin() + start, input.begin() + end);
            auto [bwt_out, idx] = BWT::transform(block);
            results[block_idx].bwt_output = std::move(bwt_out);
            results[block_idx].primary_index = idx;
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Collect results in original order
    std::vector<uint8_t> all_bwt_output;
    std::vector<uint32_t> all_primary_indices;
    all_bwt_output.reserve(input.size());
    all_primary_indices.reserve(num_blocks);

    for (auto& r : results) {
        all_bwt_output.insert(all_bwt_output.end(), r.bwt_output.begin(), r.bwt_output.end());
        all_primary_indices.push_back(r.primary_index);
    }

    return {all_bwt_output, all_primary_indices};
}

std::vector<uint8_t>
BWT::inverse_transform_blocks(const std::vector<uint8_t>& bwt_data,
                               const std::vector<uint32_t>& primary_indices,
                               size_t block_size) {
    if (bwt_data.empty() || primary_indices.empty()) {
        return std::vector<uint8_t>();
    }

    size_t num_blocks = primary_indices.size();
    std::vector<std::vector<uint8_t>> results(num_blocks);

    // Launch one thread per block — inverse transforms are fully independent
    std::vector<std::thread> threads;
    threads.reserve(num_blocks);

    for (size_t block_idx = 0; block_idx < num_blocks; block_idx++) {
        size_t start = block_idx * block_size;
        size_t end = std::min(start + block_size, bwt_data.size());

        threads.emplace_back([&bwt_data, &primary_indices, &results, block_idx, start, end]() {
            std::vector<uint8_t> block_bwt(bwt_data.begin() + start, bwt_data.begin() + end);
            results[block_idx] = BWT::inverse_transform(block_bwt, primary_indices[block_idx]);
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    // Collect in order
    std::vector<uint8_t> output;
    output.reserve(bwt_data.size());
    for (auto& r : results) {
        output.insert(output.end(), r.begin(), r.end());
    }

    return output;
}

// ============================================================================
// Helper: Validate Suffix Array
// ============================================================================

bool BWT::validate_suffix_array(const std::vector<uint8_t>& data,
                                const std::vector<uint32_t>& sa) {
    uint32_t n = data.size();

    if (sa.size() != n) {
        return false;
    }

    std::vector<bool> seen(n, false);
    for (uint32_t idx : sa) {
        if (idx >= n || seen[idx]) {
            return false;
        }
        seen[idx] = true;
    }

    for (uint32_t i = 1; i < n; i++) {
        uint32_t prev_suffix = sa[i - 1];
        uint32_t curr_suffix = sa[i];

        for (uint32_t offset = 0; offset < n; offset++) {
            uint8_t prev_char = data[(prev_suffix + offset) % n];
            uint8_t curr_char = data[(curr_suffix + offset) % n];

            if (prev_char < curr_char) {
                break;
            }

            if (prev_char > curr_char) {
                return false;
            }
        }
    }

    return true;
}
