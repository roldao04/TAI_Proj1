#include "transform/bwt.h"
#include <algorithm>
#include <stdexcept>

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

    uint32_t n = input.size();
    std::vector<uint32_t> suffix_array = build_suffix_array_prefix_doubling(input);

    std::vector<uint8_t> bwt_output(n);
    uint32_t primary_index = 0;

    for (uint32_t i = 0; i < n; i++) {
        uint32_t suffix_start = suffix_array[i];

        if (suffix_start == 0) {
            primary_index = i;
        }

        uint32_t prev_pos = (suffix_start == 0) ? (n - 1) : (suffix_start - 1);
        bwt_output[i] = input[prev_pos];
    }

    return {bwt_output, primary_index};
}

// ============================================================================
// Inverse BWT Transform
// ============================================================================

std::vector<uint8_t>
BWT::inverse_transform(const std::vector<uint8_t>& bwt_data, uint32_t primary_index) {
    if (bwt_data.empty()) {
        return std::vector<uint8_t>();
    }

    uint32_t n = bwt_data.size();

    if (primary_index >= n) {
        throw std::runtime_error("Invalid primary index for BWT inverse transform");
    }

    std::vector<uint32_t> count(256, 0);
    for (uint8_t c : bwt_data) {
        count[c]++;
    }

    std::vector<uint32_t> cumsum(256, 0);
    uint32_t sum = 0;
    for (int i = 0; i < 256; i++) {
        cumsum[i] = sum;
        sum += count[i];
    }

    // Build LF mapping: maps each position in last column to its position in first column
    std::vector<uint32_t> lf_mapping(n);
    std::vector<uint32_t> temp_cumsum = cumsum;

    for (uint32_t i = 0; i < n; i++) {
        uint8_t c = bwt_data[i];
        lf_mapping[i] = temp_cumsum[c];
        temp_cumsum[c]++;
    }

    // Reconstruct original by following LF mapping
    std::vector<uint8_t> output(n);
    uint32_t idx = primary_index;

    for (uint32_t i = 0; i < n; i++) {
        uint8_t c = bwt_data[idx];
        output[n - 1 - i] = c;
        idx = lf_mapping[idx];
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

    std::vector<uint8_t> all_bwt_output;
    std::vector<uint32_t> all_primary_indices;

    all_bwt_output.reserve(input.size());
    all_primary_indices.reserve(num_blocks);

    for (size_t block_idx = 0; block_idx < num_blocks; block_idx++) {
        size_t start = block_idx * block_size;
        size_t end = std::min(start + block_size, input.size());

        std::vector<uint8_t> block(input.begin() + start, input.begin() + end);
        auto [bwt_output, primary_index] = transform(block);

        all_bwt_output.insert(all_bwt_output.end(), bwt_output.begin(), bwt_output.end());
        all_primary_indices.push_back(primary_index);
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
    std::vector<uint8_t> output;
    output.reserve(bwt_data.size());

    for (size_t block_idx = 0; block_idx < num_blocks; block_idx++) {
        size_t start = block_idx * block_size;
        size_t end = std::min(start + block_size, bwt_data.size());
        size_t current_block_size = end - start;

        if (end > bwt_data.size()) {
            throw std::runtime_error("BWT block processing error: block boundaries exceed data size");
        }

        std::vector<uint8_t> block_bwt(bwt_data.begin() + start,
                                       bwt_data.begin() + end);

        if (block_bwt.size() != current_block_size) {
            throw std::runtime_error("BWT block processing error: block size mismatch");
        }

        std::vector<uint8_t> block_output = inverse_transform(block_bwt, primary_indices[block_idx]);
        output.insert(output.end(), block_output.begin(), block_output.end());
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
