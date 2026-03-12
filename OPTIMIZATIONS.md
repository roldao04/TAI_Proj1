# Compression Speed Optimizations

Each optimization is documented with its motivation, the research/logic behind it, and how it was implemented.

---

## Optimization 1: `-march=native` compiler flag

**File changed:** `Makefile`

### Why
`-O3` enables aggressive optimizations but still generates code that is safe to run on any x86-64 CPU. `-march=native` additionally tells the compiler it can use every instruction the *current CPU* supports — in practice this enables AVX2 (256-bit SIMD registers instead of 128-bit SSE2). Loops the compiler was already auto-vectorizing at 128-bit width now process twice as much data per clock cycle.

### Research / Logic
From the Intel Auto-Vectorization Guide and GCC documentation: `-march=native` enables target-specific instruction sets (AVX2, BMI2, POPCNT, etc.) that the compiler uses when auto-vectorizing loops over arrays. This is always safe in single-machine compilation and gives 5–20% improvement on data-intensive loops like frequency counting, cumulative sum building, and memory copies.

### How
Added `-march=native` to `CXXFLAGS` in the Makefile:
```makefile
# Before
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -I./include

# After
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -I./include
```

No source code changes required. Rebuild with `make clean && make`.

---

## Optimization 2: `__restrict__` on hot loop pointer parameters

**Files changed:** `src/transform/bwt.cpp`, `src/model/frequency_model.cpp`

### Why
When a function takes two pointer parameters, the compiler must assume they *might* point to overlapping memory (pointer aliasing). This forces it to reload values from memory between writes — it cannot keep values in registers or reorder instructions. `__restrict__` is a promise to the compiler that the pointers do not alias, which unlocks vectorization and register allocation that was blocked.

### Research / Logic
From the Intel Vectorization Guide: "The most common barrier to auto-vectorization is potential pointer aliasing. Use `__restrict__` to assert non-aliasing." This is a zero-cost annotation — it adds no instructions, only enables the compiler to generate better ones.

The hot loops in this codebase are:
- `build_from_data` in `frequency_model.cpp`: iterates over a `const uint8_t*` data array and writes to a frequency `uint32_t[]` array — these cannot alias.
- `build_suffix_array_prefix_doubling` in `bwt.cpp`: multiple working arrays (`suffix_array`, `equivalence_class`, `count`, `next_suffix_array`, `next_equivalence_class`) iterated in tight loops — none alias each other.

### How

**`frequency_model.cpp` — `build_from_data`:**
```cpp
// Before
void FrequencyModel::build_from_data(const std::vector<uint8_t>& data)

// After: extract inner hot loop into a helper with __restrict__
```
Added a static inline helper that processes the raw byte array with `__restrict__` so the compiler knows the frequency array and data buffer are independent:

```cpp
static void count_frequencies(const uint8_t* __restrict__ data,
                               size_t n,
                               uint32_t* __restrict__ freq) {
    for (size_t i = 0; i < n; i++) {
        freq[data[i]]++;
    }
}
```

**`bwt.cpp` — inner radix sort loops in `build_suffix_array_prefix_doubling`:**
The radix sort counting and placement steps use `count[]`, `suffix_array[]`, and `equivalence_class[]` — annotated with `__restrict__` in the helper functions extracted from the doubling loop.

---

## Optimization 3: libsais — O(n) suffix array construction replacing O(n log n) prefix-doubling

**Files changed:** `src/transform/bwt.cpp`, `include/transform/bwt.h`, `Makefile`
**Files added:** `include/libsais.h`, `src/libsais.c`

### Why
The BWT requires sorting all cyclic rotations of the input block. The current implementation uses prefix-doubling, which is O(n log n). libsais implements the SA-IS (Suffix Array by Induced Sorting) algorithm by Nong, Zhang & Chan (2009), which is O(n) and additionally:
- Branchless induced-sorting loops (eliminates branch mispredictions)
- Strategic `__builtin_prefetch` calls to hide memory latency
- Manual loop unrolling for instruction-level parallelism
- Provides `libsais_bwt()` which directly computes the BWT output and primary index in one call — no need to extract the last column separately

### Research / Logic
Benchmarks from the libsais GitHub repository (Ilya Grebnov) on real hardware:

| CPU | Prefix-doubling / divsufsort | libsais | Speedup |
|---|---|---|---|
| Intel i7-7820X | 9.5 MB/s | 15.7 MB/s | 1.65x |
| AMD Ryzen 5800X | 14.7 MB/s | 24.6 MB/s | 1.67x |
| AMD Ryzen 3600X | 11.2 MB/s | 17.5 MB/s | 1.57x |

Source: Nong, Zhang, Chan (2009). "Linear Time Suffix Array Construction Using D-Critical Substrings." *IEEE DCC 2009.*
Library: https://github.com/IlyaGrebnov/libsais (Apache 2.0)

### How
1. Added `libsais.h` to `include/` and `libsais.c` to `src/`
2. Updated `Makefile` to compile `libsais.c` as a C file (not C++) and link it
3. In `bwt.cpp`, replaced the call to `build_suffix_array_prefix_doubling` + manual BWT extraction with a single call to `libsais_bwt()`:

```cpp
// Before: O(n log n) — build SA, then extract last column manually
std::vector<uint32_t> suffix_array = build_suffix_array_prefix_doubling(input);
for (uint32_t i = 0; i < n; i++) {
    uint32_t prev_pos = (suffix_array[i] == 0) ? (n - 1) : (suffix_array[i] - 1);
    bwt_output[i] = input[prev_pos];
}

// After: O(n) — libsais computes BWT directly
int32_t primary_index_signed = libsais_bwt(
    input.data(), bwt_output.data(), tmp_buffer.data(), n, 0, nullptr);
```

The inverse BWT (decompression) is unchanged — it already uses O(n) LF-mapping which is optimal.

---

## Optimization 4: Parallel block processing with std::thread

**Files changed:** `src/transform/bwt.cpp`

### Why
Each BWT block is fully independent — the transform of block N does not depend on block N-1. This is the canonical data-parallel pattern. Processing blocks sequentially leaves all CPU cores idle except one. With N cores, N blocks can be transformed simultaneously, giving close to N× throughput improvement on the BWT stage (which is typically the bottleneck for large files).

### Research / Logic
This is the same design used by `pbzip2` (parallel bzip2), which achieves near-linear scaling up to ~6–8 cores (limited by memory bandwidth on dual-channel DDR4). From pbzip2's design documentation: "Each block is independently compressed, so threads can work without synchronization except at output collection."

For 900KB blocks on a typical 8-core machine, 8 blocks can be in-flight simultaneously, giving a theoretical 8× improvement, with practical throughput of ~5–7× due to memory bandwidth and output serialization overhead.

### How
Replaced the sequential block loop in `transform_blocks` with a thread pool using `std::thread`:

```cpp
// Before: sequential
for (size_t block_idx = 0; block_idx < num_blocks; block_idx++) {
    auto [bwt_output, primary_index] = transform(block);
    // collect...
}

// After: parallel — each thread processes one block
std::vector<std::thread> threads(num_blocks);
std::vector<result_t> results(num_blocks);

for (size_t i = 0; i < num_blocks; i++) {
    threads[i] = std::thread([&, i]() {
        results[i] = transform(blocks[i]);
    });
}
for (auto& t : threads) t.join();
// collect results in order
```

The output is collected in the original order after all threads finish, guaranteeing deterministic output.
`-lpthread` added to `LDFLAGS` in the Makefile.
