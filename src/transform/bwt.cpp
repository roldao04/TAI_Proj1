#include "transform/bwt.h"
#include <algorithm>
#include <stdexcept>
#include <cstring>

/*
 * BWT Implementation
 *
 * Uses naive O(n²log n) suffix array construction for cyclic rotations.
 * Block-based processing for large files (900KB blocks, similar to bzip2).
 */

// ============================================================================
// Suffix Array Construction
// ============================================================================

std::vector<uint32_t> BWT::build_suffix_array_naive(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        return std::vector<uint32_t>();
    }

    uint32_t n = data.size();
    std::vector<uint32_t> suffix_array(n);

    for (uint32_t i = 0; i < n; i++) {
        suffix_array[i] = i;
    }

    // Sort cyclic rotations lexicographically (key difference from standard suffix array)
    std::sort(suffix_array.begin(), suffix_array.end(),
        [&data, n](uint32_t i, uint32_t j) {
            for (uint32_t k = 0; k < n; k++) {
                uint8_t char_i = data[(i + k) % n];
                uint8_t char_j = data[(j + k) % n];

                if (char_i < char_j) {
                    return true;
                } else if (char_i > char_j) {
                    return false;
                }
            }
            return false;
        });

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
    std::vector<uint32_t> suffix_array = build_suffix_array_naive(input);

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
        uint32_t prev_suffix = sa[i-1];
        uint32_t curr_suffix = sa[i];

        uint32_t len_prev = n - prev_suffix;
        uint32_t len_curr = n - curr_suffix;
        uint32_t min_len = std::min(len_prev, len_curr);

        int cmp = std::memcmp(&data[prev_suffix], &data[curr_suffix], min_len);

        if (cmp > 0) {
            return false;
        }

        if (cmp == 0 && len_prev > len_curr) {
            return false;
        }
    }

    return true;
}
