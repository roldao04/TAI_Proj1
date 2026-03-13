# Version 4.0 - Detailed Documentation

**Release Date:** March 13, 2026

**Status:** Current Release

---

## Overview

Version 4.0 is a performance-focused release that preserves v3's compression pipeline (BWT + MTF + Range Coding + Order-1 context model) while dramatically accelerating it through four targeted optimizations. The most impactful change — replacing the Order-1 context model's internal data structure — yielded up to **69× speedup** on large files, transforming the tool from unacceptably slow to competitive with industry tools.

Block size was also increased from 1024 bytes (v3) to 900 KB (matching bzip2's design), improving compression ratios.

---

## What's New in v4.0

### Optimizations Applied

1. **`-march=native` compiler flag** — unlocks AVX2 SIMD auto-vectorization
2. **`__restrict__` pointer annotation** — removes aliasing barrier on hot loops
3. **libsais library** — O(n) suffix array replacing O(n log n) prefix-doubling
4. **Parallel block processing** — one thread per block via `std::thread`
5. **Flat array context model** — replaces `std::map` + dirty cache with 2D array

---

## Core Changes

### 1. `-march=native` Compiler Flag

**What changed:**
- Added `-march=native` to `CXXFLAGS` and `CFLAGS` in the Makefile

**Why:**
- `-O3` already enables aggressive optimizations, but generates code safe for any x86-64 CPU
- `-march=native` additionally allows the compiler to use every instruction the current CPU supports — in practice, AVX2 (256-bit SIMD registers vs 128-bit SSE2)
- Loops that were being auto-vectorized at 128-bit width now process twice as much data per clock cycle

---

### 2. `__restrict__` on Hot Loop Pointer Parameters

**Files changed:** `src/model/frequency_model.cpp`

**What changed:**
```cpp
// Added static helper with __restrict__ annotation:
static void count_frequencies(const uint8_t* __restrict__ data,
                               size_t n,
                               uint32_t* __restrict__ freq) {
    for (size_t i = 0; i < n; i++) {
        freq[data[i]]++;
    }
}
```

**Why:**
- Without `__restrict__`, the compiler assumes `data` and `freq` might overlap in memory (pointer aliasing). This forces it to reload from memory between writes, preventing vectorization.
- `__restrict__` is a zero-cost promise that the pointers don't alias — the compiler can now auto-vectorize the loop with AVX2.

---

### 3. libsais — O(n) Suffix Array Construction

**Files changed:** `src/transform/bwt.cpp`, `Makefile`
**Files added:** `include/libsais.h`, `src/libsais.c`

**What changed:**
```cpp
// Before: O(n log n) — build suffix array, then extract last column manually
std::vector<uint32_t> sa = build_suffix_array_prefix_doubling(input);
for (uint32_t i = 0; i < n; i++) {
    bwt_output[i] = input[(sa[i] == 0) ? n-1 : sa[i]-1];
}

// After: O(n) — libsais computes BWT directly
int32_t primary_index = libsais_bwt(
    input.data(), bwt_output.data(), tmp.data(), n, 0, nullptr);

// Inverse transform also replaced:
int32_t ret = libsais_unbwt(
    bwt_data.data(), output.data(), tmp.data(), n, nullptr, primary_index);
```

**Why:**
- The previous implementation used prefix-doubling suffix array construction: O(n log n)
- libsais implements the SA-IS algorithm (Nong, Zhang & Chan, IEEE DCC 2009): O(n) — linearly faster
- libsais also provides `libsais_bwt()` / `libsais_unbwt()` as a matched pair, computing the BWT directly without extracting the last column manually
- Library source: [github.com/IlyaGrebnov/libsais](https://github.com/IlyaGrebnov/libsais) (Apache 2.0 license)
- libsais.c is compiled with `gcc` (not `g++`) as it is a C library

---

### 4. Parallel Block Processing with `std::thread`

**Files changed:** `src/transform/bwt.cpp`, `Makefile` (`-lpthread`)

**What changed:**
```cpp
// Before: sequential — one block at a time
for (size_t block_idx = 0; block_idx < num_blocks; block_idx++) {
    results[block_idx] = BWT::transform(block[block_idx]);
}

// After: parallel — one thread per block
std::vector<std::thread> threads;
for (size_t i = 0; i < num_blocks; i++) {
    threads.emplace_back([&, i]() { results[i] = BWT::transform(block[i]); });
}
for (auto& t : threads) t.join();
// results collected in original order after all threads finish
```

Same pattern applied to `inverse_transform_blocks` for decompression.

**Why:**
- Each BWT block is fully independent — block N's transform does not depend on block N-1
- This is the same design as `pbzip2` (parallel bzip2)
- On an 8-core CPU, 8 blocks transform simultaneously, giving close to 8× BWT throughput
- Block size increased to 900 KB (from v3's 1024 bytes), matching bzip2's design

---

### 5. Flat Array Order-1 Context Model

**Files changed:** `src/model/context_model.cpp`, `include/model/context_model.h`

This was the single most impactful optimization.

**The problem with v3's design:**
- Each of the 256 order-1 contexts stored its frequencies in a `std::map<int, uint32_t>`
- On every symbol encoded, the model: traversed the map (pointer chasing, cache misses), marked a cache dirty, re-sorted a symbol list, and rebuilt a cumulative array from scratch
- File H (1 MB): 6,594 ms total — 68 ms in BWT+MTF, ~6,500 ms in the encoder

**What changed:**
```cpp
// Before: heap-allocated map per context
std::array<std::unique_ptr<Context>, 256> order1_contexts;
// Context::frequencies = std::map<int, uint32_t>
// + dirty cache rebuilt via sort + scan on every access

// After: flat 2D arrays
uint32_t freq0_[258];        // order-0: one slot per possible symbol
uint32_t freq1_[256][258];   // order-1: [context byte][symbol]
uint32_t total1_[256];       // running total per context
uint32_t seen1_[256];        // unique symbols per context (for escape freq)
bool     ctx_exists_[256];   // lazy init flag
```

**Why it works:**
- With only 258 possible symbols (0–255 + escape + EOF), a flat `uint32_t[258]` per context costs 1 KB and fits entirely in L1 cache
- Total model memory: 256 × 258 × 4 = ~264 KB — fits in L2 cache entirely
- All operations become simple array operations:
  - **Update**: two array writes + counter increment → O(1)
  - **Get range**: linear prefix sum over ≤258 integers → AVX2-vectorizable
  - **Find symbol**: linear scan over ≤258 integers → AVX2-vectorizable
- No maps, no trees, no cache rebuild, no sorting

---

## Performance Analysis

### Benchmark Results (8 test files, 13MiB total)

**Overall Metrics:**
- Average compression ratio: **56.39%**
- Total original size: 13 MiB
- Total compressed: 7.1 MiB
- Overall ranking: **#3** among 5 tools tested

**Comparison with Industry Tools:**

| Rank | Tool | Avg Ratio | Notes |
|------|------|-----------|-------|
| 🥇 #1 | xz | 54.16% | Best compression |
| 🥈 #2 | bzip2 | 54.88% | Good balance |
| 🥉 #3 | **g07 v4.0** | **56.39%** | Our tool |
| #4 | zstd | 58.58% | Very fast |
| #5 | gzip | 59.47% | Industry standard |

### Speed Improvement Over v3

The speedup is most dramatic on large, structured files (where Order-1 was slowest):

| File | v3 Compress Time | v4.0 Compress Time | Speedup |
|------|------------------|--------------------|---------|
| H (1 MB) | ~6,594 ms | **96 ms** | **69×** |
| C (2 MB) | ~2,614 ms | **96 ms** | **27×** |
| G (2.5 MB) | ~3,357 ms | **117 ms** | **29×** |
| A (1.3 MB) | — | 101 ms | — |
| B (1.2 MB) | — | 68 ms | — |
| D (2 MB, random) | — | 14 ms | — |
| E (989 KB) | — | 19 ms | — |
| F (2 MB) | — | 28 ms | — |

---

## Compressed File Format

Unchanged from v3:

```
+-------------+------------------+------------------+-----------------+---------------+
| Model Type  | Original Size    | BWT Block Count  | BWT Indices     | Encoded Data  |
| (1 byte)    | (8 bytes)        | (4 bytes)        | (4B × N blocks) | (variable)    |
+-------------+------------------+------------------+-----------------+---------------+
```

**Model Type Values:**
- `0x03` = Order-0 + BWT
- `0x04` = Order-1 + BWT
- `0x00` = Order-0 (no BWT)
- `0x01` = Order-1 (no BWT)
- `0xFF` = Uncompressed

**Block size:** 900 KB (921,600 bytes) — changed from v3's 1024 bytes.

---

## Usage

```bash
# Build
make clean && make

# Compress (auto-selects model and BWT)
./bin/compress <input> <output> [--model auto|order0|order1] [--bwt|--no-bwt] [-y]

# Decompress
./bin/decompress <compressed> <output>

# Run tests
make test

# Run benchmarks
make benchmark
```

---

## Comparison with v3

| Feature | v3 | v4.0 |
|---------|----|------|
| **BWT Algorithm** | O(n log n) prefix-doubling | O(n) libsais (SA-IS) |
| **BWT Implementation** | Own code | libsais (Apache 2.0) |
| **Block Size** | 1024 bytes | 900 KB |
| **Block Processing** | Sequential | Parallel (std::thread) |
| **Context Model Storage** | `std::map` per context | Flat `uint32_t[256][258]` array |
| **Compiler Flags** | `-O3` | `-O3 -march=native` |
| **SIMD Vectorization** | SSE2 (128-bit) | AVX2 (256-bit) |
| **Avg Compression Ratio** | 57.48% | **56.39%** |
| **File H Compress Time** | ~6,594 ms | **96 ms (69×)** |
| **File G Compress Time** | ~3,357 ms | **117 ms (29×)** |
| **Overall Rank** | #3 | **#3** |

---

## Build Information

**Compilation:**
- Compiler: g++ (C++17) for C++ sources, gcc for libsais.c
- Optimization: `-O3 -march=native`
- Threading: `-lpthread`
- Platform: Linux (Arch Linux, kernel 6.19.6)

**Binaries Included:**
- `G07-V4-C` — Compression tool
- `G07-V4-D` — Decompression tool

**Memory Usage:**
- Context model: ~264 KB (fits in L2 cache)
- BWT working buffer: O(n) per block (~900 KB max)
- Total maximum: well within 8 GB requirement

---

## Testing

**Verification Status:** ✓ All tests passed
- BWT unit tests: Perfect reconstruction verified
- Integration tests: Byte-for-byte lossless on all 8 test files
- Parallel blocks: Deterministic output guaranteed (results collected in order)

**Lossless Guarantee:** 100% byte-for-byte accuracy across all test cases.

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## References

- Nong, Zhang, Chan, "Linear Time Suffix Array Construction Using D-Critical Substrings", IEEE DCC 2009
- I. Grebnov, libsais — https://github.com/IlyaGrebnov/libsais (Apache 2.0)
- M. Burrows and D.J. Wheeler, "A block-sorting lossless data compression algorithm" (1994)
- Michael Schindler, "A Fast Renormalisation for Arithmetic Coding" (1998)
- TAI Course Materials, Universidade de Aveiro

---

## License

Educational project for TAI course at Universidade de Aveiro.

Range coder implementation: GPL v2+ (Michael Schindler)
libsais: Apache 2.0 (Ilya Grebnov)
All other code: Original group work

---

*Documentation last updated: March 13, 2026*
