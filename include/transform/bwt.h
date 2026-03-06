#ifndef BWT_H
#define BWT_H

#include <vector>
#include <cstdint>
#include <utility>

/*
 * Burrows-Wheeler Transform (BWT)
 *
 * The BWT is a block-sorting compression algorithm that rearranges
 * characters in a string into runs of similar characters. This makes
 * the output more compressible by subsequent statistical coders.
 *
 * Key Properties:
 * - Reversible: Can perfectly reconstruct original from BWT output
 * - Groups similar characters together
 * - Particularly effective on structured data (XML, JSON, etc.)
 *
 * Example:
 *   Input:  "banana$"
 *   Output: "annb$aa" + primary_index=3
 *
 * Implementation Strategy:
 * - Phase 1: Naive O(n² log n) suffix array construction (simple, works)
 * - Phase 2: SA-IS algorithm for O(n) construction (optimized)
 * - Block-based processing for large files (900KB blocks like bzip2)
 *
 * Memory Usage:
 * - Single block (900KB): ~4MB for suffix array + data
 * - Sequential processing: max 6MB at any time
 *
 * References:
 * - Burrows & Wheeler (1994): "A Block-sorting Lossless Data Compression Algorithm"
 * - Nong, Zhang, Chan (2009): "Linear Time Suffix Array Construction Using D-Critical Substrings"
 * - bzip2 source code: Practical BWT implementation
 */

class BWT {
public:
    /*
     * Forward BWT Transform (Single Block)
     *
     * Transforms input data into BWT representation.
     *
     * Algorithm:
     * 1. Append sentinel character (implicit, not stored)
     * 2. Build suffix array (all rotations sorted)
     * 3. Extract last column of sorted rotations matrix
     * 4. Return transformed data + primary index
     *
     * Parameters:
     *   input - Input data to transform
     *
     * Returns:
     *   pair<bwt_output, primary_index>
     *   - bwt_output: Transformed data (same length as input)
     *   - primary_index: Position where original string appears in sorted rotations
     *
     * Time Complexity: O(n²log n) naive, O(n) with SA-IS
     * Space Complexity: O(n)
     *
     * Example:
     *   transform("banana") → ("annbaa", 3)
     */
    static std::pair<std::vector<uint8_t>, uint32_t>
        transform(const std::vector<uint8_t>& input);

    /*
     * Inverse BWT Transform (Single Block)
     *
     * Reconstructs original data from BWT output.
     *
     * Algorithm:
     * 1. Count character frequencies
     * 2. Compute first column (sorted characters)
     * 3. Build LF (Last-to-First) mapping
     * 4. Follow transformations starting from primary_index
     * 5. Reconstruct original string
     *
     * Parameters:
     *   bwt_data - BWT transformed data
     *   primary_index - Index where original appears
     *
     * Returns:
     *   Original reconstructed data
     *
     * Time Complexity: O(n)
     * Space Complexity: O(n)
     *
     * Example:
     *   inverse_transform("annbaa", 3) → "banana"
     */
    static std::vector<uint8_t>
        inverse_transform(const std::vector<uint8_t>& bwt_data, uint32_t primary_index);

    /*
     * Forward BWT Transform (Multiple Blocks)
     *
     * Transforms large input by splitting into blocks.
     * Each block processed independently for parallelization potential.
     *
     * Parameters:
     *   input - Input data to transform
     *   block_size - Size of each block (default: 900KB like bzip2)
     *
     * Returns:
     *   pair<concatenated_bwt_output, vector_of_primary_indices>
     *
     * Example:
     *   10MB file with 900KB blocks → 12 blocks, 12 primary indices
     */
    static std::pair<std::vector<uint8_t>, std::vector<uint32_t>>
        transform_blocks(const std::vector<uint8_t>& input, size_t block_size = 900*1024);

    /*
     * Inverse BWT Transform (Multiple Blocks)
     *
     * Reconstructs original from block-based BWT.
     *
     * Parameters:
     *   bwt_data - Concatenated BWT data from all blocks
     *   primary_indices - Vector of primary indices (one per block)
     *   block_size - Size of each block (must match transform)
     *
     * Returns:
     *   Original reconstructed data
     */
    static std::vector<uint8_t>
        inverse_transform_blocks(const std::vector<uint8_t>& bwt_data,
                                 const std::vector<uint32_t>& primary_indices,
                                 size_t block_size = 900*1024);

    /*
     * Build Suffix Array (Naive Implementation)
     *
     * Constructs suffix array using straightforward O(n²log n) approach.
     * Used in Phase 1 for correctness, then replaced by SA-IS.
     *
     * Algorithm:
     * 1. Generate all suffix indices (0 to n-1)
     * 2. Sort them using std::sort with custom comparator
     * 3. Comparator compares suffixes lexicographically
     *
     * Parameters:
     *   data - Input data
     *
     * Returns:
     *   Suffix array (indices into data)
     *
     * Time Complexity: O(n²log n) - comparison is O(n), done O(n log n) times
     * Space Complexity: O(n)
     *
     * Note: This is simple but slow. Will be replaced by SA-IS for O(n) time.
     */
    static std::vector<uint32_t> build_suffix_array_naive(const std::vector<uint8_t>& data);

    /*
     * Build Suffix Array (SA-IS Algorithm)
     *
     * Linear-time suffix array construction using Induced Sorting.
     * Significantly faster than naive approach, especially for large blocks.
     *
     * Algorithm (simplified):
     * 1. Classify suffixes as S-type or L-type
     * 2. Find LMS (Left-Most S-type) substrings
     * 3. Sort LMS substrings
     * 4. Induce sort remaining suffixes
     * 5. Recursively solve if needed
     *
     * Parameters:
     *   data - Input data
     *
     * Returns:
     *   Suffix array (indices into data)
     *
     * Time Complexity: O(n)
     * Space Complexity: O(n)
     *
     * Reference: Nong, Zhang, Chan (2009)
     * Note: Complex algorithm, will implement in Phase 2
     */
    static std::vector<uint32_t> build_suffix_array_sais(const std::vector<uint8_t>& data);

private:
    /*
     * Helper: Check if suffix array is valid
     *
     * Verifies that suffix array correctly represents all suffixes in sorted order.
     * Used for testing and validation.
     */
    static bool validate_suffix_array(const std::vector<uint8_t>& data,
                                      const std::vector<uint32_t>& sa);
};

#endif // BWT_H
