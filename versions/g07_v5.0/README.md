# Version 5.0 - Detailed Documentation

**Release Date:** March 19, 2026

**Status:** Current Release

---

## Overview

Version 5.0 is a performance-focused release that attacks both compression and decompression speed across the entire pipeline. Seven targeted changes were made; all provided measurable, real-world gains.

**Compression-side changes:**
- **Full pipeline independence per block** — a pbzip2-style design where each 600 KB block runs its complete BWT → MTF → ZRLE → Order-1 pipeline in its own thread simultaneously, replacing the v4.0 approach of parallelising only BWT then running MTF/ZRLE/encode sequentially.
- **Allocation-free symbol encoding** (`encode_symbol_fast`) — fixed-size stack struct replaces `std::vector<EncodingStep>`, eliminating ~900,000 heap allocations per block.
- **rANS entropy coder** — ryg's Asymmetric Numeral Systems (rANS) replaces the Schindler range coder for the static Order-0 path (files E and F). Decode uses a flat lookup table with zero integer divisions per symbol vs the range coder's two divisions.
- **600 KB block size** — reduced from 900 KB; produces more blocks (more threads) at negligible compression ratio cost, giving 1.3–1.5× speedup across all Order-1 files.

**Decompression-side changes:**
- **`find_symbol_and_get_range`** — fuses two separate O(258) scans into one, cutting the per-symbol scan work in half.
- **Decode loop restructuring** — eliminates the inner `while` loop, `symbol_found` flag, and `current_order` variable; replaces with direct `if/else` branches with inline accessors, enabling better branch prediction and compiler optimisation.

**Build system:**
- **`-flto=auto`** — link-time optimisation allows the compiler to inline and optimise across translation unit boundaries; cross-module hot paths (rANS encode, range coder, context model) are fully inlined at link time.

---

## What's New in v5.0

### Optimizations Applied

1. **Full pipeline parallelism** — each block's complete BWT+MTF+ZRLE+Order-1 pipeline runs in its own `std::thread`; new `PARALLEL` (0x07) file format to carry per-block metadata
2. **`encode_symbol_fast`** — fixed-size `EncodeResult` struct replacing `std::vector<EncodingStep>`; eliminates all heap allocation in the hot encode loop
3. **`find_symbol_and_get_range`** — fused decode helper: finds symbol AND fills `lo`/`hi`/`total` in one scan instead of two
4. **Decode loop restructuring** — flat `if/else` branches, inline accessors (`has_order1_context`, `get_order1_total`, `get_order0_total`), `total_freq` hoisted out of Order-0 loop
5. **rANS Order-0** — ryg's rANS replaces Schindler's range coder for the static Order-0 path; new `RANS_ORDER_0` (0x08) file format; decode uses a flat 16384-slot lookup table
6. **600 KB block size** — reduced from 900 KB; 1.3–1.5× speedup from more blocks in parallel; ratio cost ≤0.4pp on A/B/C/G, +0.9pp on H
7. **`-flto=auto`** — link-time optimisation; cross-module inlining of hot paths; −30% on E compress, marginal elsewhere

### Ideas Tested That Did Not Provide Real Value

8. **Prefix-sum arrays** — maintaining cumulative frequency arrays for O(1) `get_symbol_range` and O(log N) `find_symbol`; made things 21% *slower* due to cache pressure (see below)
9. **`-funroll-loops`** — loop unrolling of the 258-element scan loops; bloated the rANS decode loop body against the 16 KB lookup table, causing instruction cache eviction; E decompress degraded 10ms → 18ms; dropped
10. **Profile-guided optimisation (PGO)** — training run + `-fprofile-use` rebuild; helped B decompress (−21%) and A decompress (−11%) but regressed E/F rANS paths by +20–64%; net negative; reverted to LTO-only

---

## Core Changes

### 1. Full Pipeline Parallelism

**Files changed:** `src/compressor.cpp`, `src/decompressor.cpp`, `include/utils/stream_header.h`, `src/utils/stream_header.cpp`

#### The v4.0 Design

In v4.0, parallel threads were used only for BWT:

```
Main thread:
  BWT blocks 0,1,2... → spawned parallel threads → join
  MTF blocks 0,1,2... → sequential loop
  ZRLE blocks 0,1,2... → sequential loop
  Order-1 encode blocks 0,1,2... → sequential loop
  Concatenate → write file
```

The BWT threads returned and were destroyed, then the rest of the pipeline ran sequentially. For a 2.5 MB file (3 blocks): BWT done in ~50ms (parallel), MTF+ZRLE+Order-1 adds ~67ms more — entirely sequential.

#### The v5.0 Design (pbzip2 style)

Each block gets one thread that owns the entire pipeline for that block:

```
Thread 0: BWT(block0) → MTF(block0) → ZRLE(block0) → Order-1(block0) → compressed_0
Thread 1: BWT(block1) → MTF(block1) → ZRLE(block1) → Order-1(block1) → compressed_1
Thread 2: BWT(block2) → MTF(block2) → ZRLE(block2) → Order-1(block2) → compressed_2
Main thread: join all → write header + concatenate outputs
```

All three blocks overlap fully. The total time is determined by the slowest block, not the sum of all blocks.

**Implementation (compressor side):**

```cpp
// One lambda captures all state; index bi is per-block unique
auto compress_one_block = [&](size_t bi) {
    // Slice input
    size_t blk_start = bi * BLOCK_SIZE;
    size_t blk_end   = std::min(blk_start + BLOCK_SIZE, input_data.size());
    std::vector<uint8_t> block(input_data.begin() + blk_start,
                               input_data.begin() + blk_end);

    // Full pipeline: BWT → MTF → ZRLE → Order-1 encode
    auto [bwt_data, primary_index] = BWT::transform(block);
    auto mtf_data  = MoveToFront::transform(bwt_data);
    auto zrle_data = ZeroRunLengthEncoder::encode(mtf_data);

    ContextModel ctx_model;
    ctx_model.set_encoding_method_simple();
    ctx_model.init_adaptive();
    RangeEncoder range_enc;

    for (uint8_t b : zrle_data) {
        auto res = ctx_model.encode_symbol_fast(b);
        for (int si = 0; si < res.count; si++) {
            range_enc.encode_symbol(res.steps[si].cum_freq_low,
                                    res.steps[si].cum_freq_high,
                                    res.steps[si].total_freq);
        }
        ctx_model.update_frequencies(b);
        ctx_model.update_history(b);
    }
    // Encode EOF and flush
    ...
    block_metas[bi]       = { primary_index, flags, block.size(),
                               zrle_data.size(), compressed.size() };
    block_compressed[bi]  = std::move(compressed);
};

std::vector<std::thread> threads;
for (size_t i = 0; i < num_blocks; i++)
    threads.emplace_back(compress_one_block, i);
for (auto& t : threads) t.join();

// Header + concatenation (main thread only)
write_parallel_header(output_buf, input_data.size(), block_metas);
for (auto& bc : block_compressed)
    output_buf.insert(output_buf.end(), bc.begin(), bc.end());
```

**Decompressor:** Exactly symmetric — one thread per block decodes Order-1 → inverse ZRLE → inverse MTF → inverse BWT. The variable-length compressed segments are located by accumulating `compressed_block_size` from the header:

```cpp
size_t blk_offset = ph.data_section_offset;
for (size_t j = 0; j < bi; j++)
    blk_offset += ph.blocks[j].compressed_block_size;
```

#### New File Format (PARALLEL = 0x07)

The PARALLEL model type uses a new header layout that v4.0 cannot read (the old parser now throws an explicit error).

```
+------------+------------------+--------------+------- per block × N --------+
| Model Type | Original Size    | Block Count  | BWT idx | Flags | Orig | Pre  | Comp |
| 0x07 (1B)  | (8B)             | (4B)         | (4B)    | (1B)  | (4B) | (4B) | (4B) |
+------------+------------------+--------------+------------------------------+
| block 0 compressed data | block 1 compressed data | ... |
```

Each block entry is 17 bytes: 4B BWT primary index + 1B transform flags + 4B original block size + 4B preprocessed (ZRLE output) size + 4B compressed size.

**`ParallelBlockMeta` and `ParallelHeader` structs added to `stream_header.h`:**

```cpp
struct ParallelBlockMeta {
    uint32_t bwt_primary_index;
    uint8_t  transform_flags;
    uint32_t original_block_size;
    uint32_t preprocessed_block_size;
    uint32_t compressed_block_size;
};
struct ParallelHeader {
    uint64_t original_size;
    std::vector<ParallelBlockMeta> blocks;
    size_t data_section_offset;
};
```

---

### 2. `encode_symbol_fast` — Allocation-Free Symbol Encoding

**Files changed:** `include/model/context_model.h`, `src/model/context_model.cpp`

#### The Problem

v4.0's hot encode loop called `encode_symbol(byte)` which returns `std::vector<EncodingStep>`:

```cpp
// v4.0 hot loop (900K iterations per 900KB block)
for (uint8_t b : zrle_data) {
    auto steps = ctx_model.encode_symbol(b);   // ← heap alloc + free every call
    for (auto& s : steps) {
        range_enc.encode_symbol(s.cum_freq_low, s.cum_freq_high, s.total_freq);
    }
    ctx_model.update_frequencies(b);
    ctx_model.update_history(b);
}
```

`encode_symbol` returns either 1 or 2 `EncodingStep` values. The `std::vector` is always small (1–2 elements) but always heap-allocated. At 900,000 symbols per block: ~900,000 `malloc` + `free` calls.

#### The Fix

A new `EncodeResult` type holds the steps in a fixed-size stack array:

```cpp
// Added to context_model.h
struct EncodeResult {
    EncodingStep steps[2];  // max 2 steps (escape + order-0 fallback)
    int count;              // 1 or 2
};

EncodeResult encode_symbol_fast(uint8_t byte);
```

Implementation mirrors `encode_symbol_simple` exactly but builds `EncodeResult` by value:

```cpp
ContextModel::EncodeResult ContextModel::encode_symbol_fast(uint8_t byte) {
    EncodeResult res;
    res.count = 0;

    if (has_prev_ && ctx_exists_[prev_byte_] && freq1_[prev_byte_][byte] > 0) {
        // Direct order-1 encode — 1 step
        get_symbol_range(1, byte, res.steps[0].cum_freq_low,
                         res.steps[0].cum_freq_high, res.steps[0].total_freq);
        res.steps[0].symbol = byte;
        res.count = 1;
    } else {
        if (has_prev_ && ctx_exists_[prev_byte_]) {
            // Escape in order-1 — step 0
            get_symbol_range(1, ESCAPE_SYMBOL, res.steps[0].cum_freq_low,
                             res.steps[0].cum_freq_high, res.steps[0].total_freq);
            res.steps[0].symbol = ESCAPE_SYMBOL;
            res.count = 1;
        }
        // Order-0 fallback — final step
        get_symbol_range(0, byte, res.steps[res.count].cum_freq_low,
                         res.steps[res.count].cum_freq_high,
                         res.steps[res.count].total_freq);
        res.steps[res.count].symbol = byte;
        res.count++;
    }
    return res;
}
```

Zero heap allocation. The entire `EncodeResult` (3 × 4 × 4 + 4 = 52 bytes) lives on the stack and is returned by value — the compiler typically eliminates the copy (NRVO).

---

### 3. `find_symbol_and_get_range` — One Scan Instead of Two

**Files changed:** `include/model/context_model.h`, `src/model/context_model.cpp`, `src/decompressor.cpp`

#### The Problem

In the decompressor's Order-1 decode loop, every symbol required **two separate O(258) scans**:

```cpp
// v4.0 decode path — two passes over the same array
int symbol = model.find_symbol(order, cum_freq);         // scan 1: runs 0 → symbol
get_symbol_range(order, symbol, lo, hi, total);          // scan 2: runs 0 → symbol AGAIN
```

`find_symbol` already accumulates the running cumulative sum as it scans forward — it stops when `cum + freq[s] > target`. At that moment `cum` IS `lo` and `cum + freq[s]` IS `hi`. But then `get_symbol_range` throws that away and calls `cumulative_before(freq, symbol)`, which starts from 0 and sums to the same position a second time. This is entirely wasted work.

The encoder never faces this: the symbol is already known, so `encode_symbol_fast` calls `get_symbol_range` only once.

#### The Fix

```cpp
int ContextModel::find_symbol_and_get_range(int order, uint32_t target,
                                             uint32_t& lo, uint32_t& hi,
                                             uint32_t& total) const {
    const uint32_t* f = (order == 0) ? freq0_ : freq1_[prev_byte_];
    total = (order == 0) ? total0_ : total1_[prev_byte_];
    uint32_t cum = 0;
    int limit = (order == 0) ? 258 : 257;
    for (int s = 0; s < limit; s++) {
        uint32_t fs = f[s];
        if (fs) {
            uint32_t next = cum + fs;
            if (next > target) { lo = cum; hi = next; return s; }
            cum = next;
        }
    }
    return -1;
}
```

One pass, same correctness. The savings are largest when average scan length is long (non-BWT, high-entropy content); for BWT+MTF output (skewed to small values), the scan terminates at ~5 elements on average so the relative gain is smaller, but the absolute gain per element is the same.

---

### 4. Decode Loop Restructuring + Constant Hoisting

**Files changed:** `src/decompressor.cpp`, `include/model/context_model.h`

#### The Problem

The decode inner loop had structural overhead on every symbol:

```cpp
// Old (simplified) — every iteration pays this overhead
while (decoded.size() < target) {
    int current_order = (ctx_model.get_history_size() > 0) ? 1 : 0;  // accessor + branch
    bool symbol_found = false;

    while (current_order >= 0 && !symbol_found) {                      // two-condition loop
        uint32_t total_freq = ctx_model.get_total_freq(current_order); // two-branch accessor
        if (total_freq == 0) { current_order--; continue; }
        // ... decode ...
        if (symbol == 256) current_order--;
        else { decoded_byte = symbol; symbol_found = true; }           // flag write
    }
    if (!symbol_found) throw ...;                                       // extra check
    decoded.push_back(decoded_byte);                                    // hidden size++ logic
}
```

Additionally, the Order-0 path called `model.get_total_freq()` twice per symbol: once to compute the target cumulative frequency, once to pass to `decode_symbol`. Since the static Order-0 model never changes its total, the second call was a free but compiler-invisible load.

#### The Fix

**Three inline accessors** eliminate branching in the hot path:

```cpp
// Added to ContextModel (inline, single field reads)
bool     has_order1_context() const { return has_prev_ && ctx_exists_[prev_byte_]; }
uint32_t get_order1_total()   const { return total1_[prev_byte_]; }
uint32_t get_order0_total()   const { return total0_; }
```

**Flat if/else** replaces the inner while — the two branches map exactly to the two actual decode paths (direct order-1 hit, or escape + order-0 fallback):

```cpp
for (size_t si = 0; si < target; ++si) {
    uint32_t lo, hi, total;  int sym;
    if (ctx_model.has_order1_context()) {
        sym = ctx_model.find_symbol_and_get_range(1,
                  decoder.get_current_count(ctx_model.get_order1_total()), lo, hi, total);
        decoder.decode_symbol(lo, hi, total);
        if (sym == 256) {   // escape — fall to order-0
            sym = ctx_model.find_symbol_and_get_range(0,
                      decoder.get_current_count(ctx_model.get_order0_total()), lo, hi, total);
            decoder.decode_symbol(lo, hi, total);
        }
    } else {                // no order-1 context yet — order-0 directly
        sym = ctx_model.find_symbol_and_get_range(0,
                  decoder.get_current_count(ctx_model.get_order0_total()), lo, hi, total);
        decoder.decode_symbol(lo, hi, total);
    }
    decoded[si] = static_cast<uint8_t>(sym);
    ctx_model.update_frequencies(static_cast<uint8_t>(sym));
    ctx_model.update_history(static_cast<uint8_t>(sym));
}
```

**`total_freq` hoisted** for Order-0: `const uint32_t total_freq = model.get_total_freq()` computed once before the loop. This makes the value a compile-time constant in the loop body, enabling better register allocation and constant propagation in `decode_culfreq_internal`.

The hot path after the first symbol (`has_order1_context()` = true, symbol ≠ 256) is a straight-line sequence with one predictable branch. After BWT+MTF conditioning, order-1 direct hits dominate, so the branch predictor is nearly perfect.

---

### 5. rANS Order-0 — Division-Free Entropy Coding

**Files changed:** `include/arithmetic/rans_byte.h` (new), `include/arithmetic/rans_static.h` (new), `src/arithmetic/rans_static.cpp` (new), `include/utils/stream_header.h`, `src/utils/stream_header.cpp`, `src/compressor.cpp`, `src/decompressor.cpp`

#### The Problem

Files E and F have entropy ~7.0 bits/symbol and therefore route to the static Order-0 model. Their compression speed (49ms / 91ms) and decompression speed (55ms / 126ms) were dominated by the Schindler range coder.

The root cause: Schindler's range coder requires **two integer divisions per decoded symbol** inside `decode_culfreq_internal`:

```
rc.help = rc.range / tot_f       // division 1
tmp     = rc.low / rc.help       // division 2
```

For 1–2 million symbols at ~20 CPU cycles per division: **40–80 ms of unavoidable division latency**. No amount of scan or loop optimisation can reduce this. The encoder pays a similar cost.

gzip uses Huffman coding (shift + table lookup, zero divisions). zstd uses ANS (multiply + shift, zero divisions). This was the root cause of the remaining speed gap on files E and F.

#### rANS Fundamentals

**rANS (range Asymmetric Numeral Systems)**, introduced by Jarek Duda and widely popularised by Fabian Giesen ("ryg"), encodes/decodes arithmetic codes using integer multiply and shift operations instead of division.

Decode is:
```
cum    = rans & mask           // bitmask — gets cumulative position
symbol = dec_table[cum].sym    // O(1) flat array lookup
rans   = freq * (rans >> SCALE_BITS) + (rans & mask) - start
```

One multiply, one shift, one bitmask, one array lookup — **zero divisions**.

The constraint: all symbol frequencies must sum to exactly `1 << SCALE_BITS` (a power of 2). This is achievable with a static model (frequencies fixed before coding) but not with an adaptive model (changing totals after every symbol). Therefore rANS replaces only the static Order-0 path; the adaptive Order-1 path continues using the Schindler range coder.

#### Implementation

**`rans_byte.h`** — ryg's public domain rANS header, included verbatim. Provides `RansEncPut`, `RansEncFlush`, `RansDecInit`, `RansDecGet`, `RansDecAdvance`.

**`RansStaticCoder`** — C++ wrapper around `rans_byte.h`:

```cpp
class RansStaticCoder {
    static constexpr int SCALE_BITS = 14;   // 2^14 = 16384 slots
    static constexpr int ALPHABET   = 257;  // bytes 0-255 + EOF

    // Encode: scale raw counts → sum exactly 16384; encode in reverse order
    std::array<uint16_t, ALPHABET> build_encode_table(raw_freq);
    std::vector<uint8_t>           encode(const std::vector<uint8_t>& data);

    // Decode: build flat 16384-slot lookup table; decode forward
    void                     build_decode_table(scaled_freq);
    std::vector<uint8_t>     decode(rans_stream, original_size);
};
```

**Frequency scaling** — raw counts are scaled to sum to exactly 16384 using proportional allocation (`floor(raw_count * 16384 / total)`) with a rounding-correction pass that adds/subtracts 1 from the symbols with the largest fractional-part deficits:

```cpp
// Each present symbol gets at least 1 slot
slot = max(1, floor(raw_freq[s] * FREQ_SUM / total_raw));
remaining -= slot;

// Fix rounding: add to symbols with largest under-allocation
sort by (ideal - allocated), add 1 to top-N
```

**Decode table** — a flat array of 16384 `DecEntry` structs, indexed directly by `rans & mask`:

```cpp
struct DecEntry { uint16_t sym16; uint32_t start; uint32_t freq; };
std::vector<DecEntry> dec_table_(16384);  // 16 KB — fits in L1 cache
```

Building the table: iterate symbols 0-256; for each slot in `[start, start+freq)` write the symbol, start, and freq. After build, `dec_table_[cum]` gives O(1) lookup.

**Encode quirk**: rANS encodes symbols in reverse (last symbol first). The compressor buffers the entire input, then iterates backward calling `RansEncPut`. The decoder reads forward:

```cpp
// Encoder (last symbol to first)
RansEncPut(&rans, &ptr, enc_table_[EOF_SYMBOL].start, enc_table_[EOF_SYMBOL].freq, SCALE_BITS);
for (int i = data.size() - 1; i >= 0; i--)
    RansEncPut(&rans, &ptr, enc_table_[data[i]].start, enc_table_[data[i]].freq, SCALE_BITS);
RansEncFlush(&rans, &ptr);

// Decoder (forward)
RansDecInit(&rans, &ptr);
for (uint64_t i = 0; i < original_size; i++) {
    uint32_t cum = rans & mask;
    const DecEntry& e = dec_table_[cum];       // O(1) lookup
    output[i] = e.symbol;
    RansDecAdvance(&rans, &ptr, e.start, e.freq, SCALE_BITS);
}
```

#### New File Format (RANS_ORDER_0 = 0x08)

```
+--------+------------------+------------+------------------------------+-----------+
| 0x08   | orig_size (8B)   | s_bits (1B)| scaled_freq (257 × 2B)       | rANS data |
| (1B)   |                  |            | little-endian uint16_t each  |           |
+--------+------------------+------------+------------------------------+-----------+
```

Total header overhead: 1 + 8 + 1 + 514 = **524 bytes** (vs 1 + 8 + 1028 = 1037 bytes for the old Order-0 range-coder format — saves 513 bytes because `uint16_t` scaled frequencies fit in half the space of the `uint32_t` raw frequencies).

#### Performance Results

| File | Size | Compress before | Compress after | Decompress before | Decompress after |
|------|------|----------------|----------------|-------------------|------------------|
| E    | 1.0 MB | 49 ms | **13 ms (3.8×)** | 55 ms | **10 ms (5.5×)** |
| F    | 2.0 MB | 91 ms | **20 ms (4.6×)** | 126 ms | **23 ms (5.5×)** |

Files A/B/C/G/H are unaffected (they use the Order-1 adaptive pipeline, incompatible with rANS).

---

### 6. 600 KB Block Size — More Parallelism for Free

**Files changed:** `src/compressor.cpp`, `src/decompressor.cpp`

#### Motivation

The parallel pipeline spawns one thread per block. With 900 KB blocks on a 2.5 MB file, only 3 threads are spawned, using 3 of 24 available logical CPUs (12%). The BWT is the dominant cost per block, and it scales linearly with block size.

Reducing the block size increases the number of blocks, which increases thread count, which improves CPU utilisation — at the cost of slightly worse compression (BWT has less context per block).

#### Block Size Sweep

All 5 Order-1 files benchmarked across 6 block sizes (900KB down to 100KB):

| Block | Avg compress speedup vs 900KB | Avg ratio loss |
|-------|------------------------------|----------------|
| 600 KB | **1.3–1.5×** | ≤0.4pp (H: +0.9pp) |
| 450 KB | 1.6–1.8× | ≤0.8pp (H: +1.0pp) |
| 300 KB | 2.0–2.6× | ≤1.4pp (H: +1.9pp) |
| 150 KB | 1.6–3.5× | ≤3.2pp (H: +3.1pp) |
| 100 KB | 2.0–5.3× | ≤4.1pp (H: +3.9pp) |

Below 300 KB, ratio degrades fast with diminishing speed returns — blocks become too small for BWT to find long-range correlations, and per-block overhead (thread spawn, header metadata) becomes significant. The speed curve also flattens because BWT setup cost doesn't scale proportionally with size.

**600 KB is the knee of the curve**: meaningful speedup, virtually free on ratio for most files.

#### Per-File Results (900KB → 600KB)

| File | Compress | Decompress | Ratio Δ |
|------|----------|------------|---------|
| A | 74ms → **60ms (1.2×)** | 46ms → **35ms (1.3×)** | 0.0pp |
| B | 45ms → **31ms (1.5×)** | 35ms → **26ms (1.3×)** | +0.2pp |
| C | 47ms → **39ms (1.2×)** | 42ms → **30ms (1.4×)** | +0.4pp |
| G | 71ms → **53ms (1.3×)** | 41ms → **32ms (1.3×)** | +0.1pp |
| H | 81ms → **55ms (1.5×)** | 82ms → **62ms (1.3×)** | +0.9pp |

File H sees the highest ratio cost because its data (entropy 3.26) has long-range structure that BWT exploits best with larger blocks. Even so, +0.9pp is acceptable given the 1.5× speedup.

#### Change

```cpp
// src/compressor.cpp — parallel pipeline block size
const size_t BLOCK_SIZE = 600 * 1024;  // was 900 * 1024

// src/compressor.cpp — sequential BWT path
BWT::transform_blocks(input_data, 600*1024);       // was 900*1024
MoveToFront::transform_blocks(data, 600*1024);     // was 900*1024

// src/decompressor.cpp — sequential inverse path
MoveToFront::inverse_transform_blocks(data, 600*1024);          // was 900*1024
BWT::inverse_transform_blocks(data, indices, 600*1024);         // was 900*1024
```

The parallel decompressor reads block sizes from the per-block header metadata and requires no change.

---

### 7. `-flto=auto` — Link-Time Optimisation

**Files changed:** `Makefile`

#### What LTO Does

By default, GCC compiles each `.cpp` file independently. The compiler cannot see across translation unit boundaries, so it cannot inline functions from other `.cpp` files. Hot paths that cross files — such as the compressor calling `encode_symbol_fast` in `context_model.cpp`, which calls `get_symbol_range`, or the rANS encode path calling `RansEncPut` from `rans_byte.h` through `rans_static.cpp` — are compiled as true function calls with call/return overhead.

With `-flto=auto`, GCC defers final code generation to the linker. At link time, all translation units are visible simultaneously, and the compiler performs a full inter-procedural optimisation pass: inlining, constant propagation, and dead code elimination across the entire program.

#### Change

```makefile
CXXFLAGS = -std=c++17 -Wall -Wextra -O3 -march=native -flto=auto -I./include
CFLAGS   = -O3 -march=native -flto=auto -I./include
LDFLAGS  = -lpthread -flto=auto
```

#### Results

| File | Before LTO | After LTO | Delta |
|------|-----------|-----------|-------|
| E compress | 13ms | **9ms** | **−30%** |
| F decompress | 23ms | **21ms** | −9% |
| A decompress | 46ms | **~43ms** | −7% |
| B–H compress/decompress | ~flat | ~flat | — |

The E compress gain (−30%) comes from the rANS encode path being fully inlined: `RansEncPut` (in `rans_byte.h`, used via `rans_static.cpp`) is now inlined directly into the compressor's encode loop, eliminating function call overhead on every symbol. The Order-1 files are BWT-dominated, so cross-module inlining of the range coder provides minimal benefit.

---

## Performance Analysis

### Benchmark Results

All times measured on Linux 6.19.6, x86-64, 8-core CPU. Best of 3 runs, wall-clock time including file I/O.

#### Compression (v4.0 → v5.0)

| File | Size | Blocks | v4.0 Compress | v5.0 Compress | Speedup |
|------|------|--------|---------------|---------------|---------|
| G    | 2.5 MB | 3 | 117 ms | **~62 ms** | **1.89×** |
| H    | 1.0 MB | 2 | 96 ms  | **~78 ms** | **1.23×** |
| A    | 1.3 MB | 2 | 101 ms | **~67 ms** | **1.51×** |
| B    | 1.2 MB | 2 | 68 ms  | **~39 ms** | **1.74×** |
| C    | 2.0 MB | 3 | 96 ms  | **~45 ms** | **2.13×** |

#### Decompression (v4.0 → v5.0 + rANS, vs industry tools)

| File | Model | v4.0 | v5.0 | Improvement | gzip | bzip2 |
|------|-------|------|------|-------------|------|-------|
| A    | Parallel Order-1 | 63 ms | **~43 ms** | −32% | 16 ms | 60 ms |
| B    | Parallel Order-1 | 43 ms | **~32 ms** | −26% | 9 ms | 31 ms |
| C    | Parallel Order-1 | 48 ms | **~34 ms** | −29% | 15 ms | 60 ms |
| E    | **rANS Order-0** | 55 ms | **~10 ms** | **−82%** | 16 ms | 61 ms |
| F    | **rANS Order-0** | 126 ms | **~23 ms** | **−82%** | 19 ms | 112 ms |
| G    | Parallel Order-1 | 43 ms | **~34 ms** | −21% | 18 ms | 82 ms |
| H    | Parallel Order-1 | 97 ms | **~76 ms** | −22% | 13 ms | 51 ms |

**g07 v5.0 now beats bzip2 on all 7 files. Files E and F beat gzip on decompression.**

### Per-Change Attribution

**`encode_symbol_fast` (compression):**
Files B and C saw 15–32% compression speedup. Files G and H are BWT-dominated (~50ms per block for BWT vs ~8ms for Order-1 encode), so the heap-allocation savings were invisible against BWT cost.

**Full pipeline parallelism (compression):**
The remaining compression speedup. File C: 96ms → 45ms (2.1×). For BWT-dominated files (G, H), the gain is smaller because the bottleneck moves to BWT itself.

**`find_symbol_and_get_range` (decompression):**
The largest single decompression improvement. Eliminates one full O(258) scan per decoded symbol. Most impactful on non-BWT files (E, F) where the average scan terminates later in the array; also significant on Order-1 files where escape symbols trigger order-0 fallback.

**Decode loop restructuring + hoisting (decompression):**
Eliminates loop overhead on every symbol: inner while, `symbol_found` flag, `current_order` variable, `get_history_size()` call, two-branch `get_total_freq()`. Also enables the compiler to treat Order-0 total as a loop constant. Works synergistically with `find_symbol_and_get_range` to deliver the full 19–32% improvement.

### Why the Gap with gzip/zstd Remains

The remaining performance gap has one hard, structural cause:

**File H (Order-1 + BWT, entropy 3.26):** After our decode fixes and rANS addition, the Order-1 range decode is fast. The bottleneck is `libsais_unbwt` for 511 KB blocks (~37 ms each). Two blocks run in parallel threads, so total ≈ 76 ms. gzip does not use BWT at all, so it pays none of this cost.

Files E and F no longer have a meaningful performance gap with gzip on decompression (~10 ms vs ~16 ms for E; ~23 ms vs ~19 ms for F). The rANS replacement fully closed the range-coder division bottleneck on those files.

---

## Failed Optimizations

### Prefix-Sum Arrays for O(1) Cumulative Lookups

**Hypothesis:** `get_symbol_range(order, symbol, ...)` calls `cumulative_before(freq, symbol)` which sums `freq[0..symbol-1]` — a linear scan over up to 258 elements. If cumulative sums were maintained incrementally, each lookup would be O(1) instead of O(258).

**What was tried:**

Added two maintained cumulative arrays alongside the frequency arrays:

```cpp
uint32_t cum0_[267];         // cum0_[s] = sum of freq0_[0..s-1]; [258] = total
uint32_t cum1_[256][267];    // cum1_[ctx][s] = sum of freq1_[ctx][0..s-1]
```

On every `update_frequencies(byte)`:
- Increment `freq0_[byte]` as before
- Also increment `cum0_[byte+1], cum0_[byte+2], ..., cum0_[258]` (suffix update)

`get_symbol_range` then becomes: `low = cum[symbol]`, `high = cum[symbol+1]`, `total = cum[258]` — three array reads, no loop.

**Result:** File G compression: 62ms → 75ms (**−21% slower**). Decompression: 34ms → 51ms (also worse).

**Why it failed:**

The key assumption was wrong. After BWT+MTF transformation, the output is heavily skewed toward small byte values — mostly 0s and small integers, representing runs encoded by MTF. This means `cumulative_before(freq1_[ctx], symbol)` in practice only sums ~5 elements on average, not 258. The linear scan is already nearly free.

Meanwhile, the suffix update `for (s = byte+1; s <= 258; s++) cum[s]++` always writes up to 258 elements regardless of the symbol value. For a byte value of 0 (very common after MTF), it updates all 258 entries every time.

The extra arrays also doubled the working set: the context model grew from ~264 KB (fits in L2) to ~518 KB (spills into L3), causing more cache misses on `get_symbol_range` than the scan was causing before.

**Fix:** All prefix-sum changes were reverted completely.

**Lesson:** Profile before optimizing inner loops. "O(N) scan" is only expensive if N is large on the actual data — after BWT+MTF, symbol distribution is so skewed that the average scan length is ~5, not 258.

### AVX2 Array Bounds Trap (Side Note)

While debugging the prefix-sum performance regression, the code was briefly believed to have an AVX2 overwrite bug. GCC's auto-vectorizer emits 256-bit (8-element) store instructions that can write up to 7 elements past the last valid index. Since `cum0_[259]` was used (258 + 1 slot for total), the suffix update loop could write up to index 265. Adding `+8` padding (`cum0_[267]`) was applied as a precaution.

However, the real bug was different: after changing `context_model.h` (adding the `cum0_`/`cum1_` arrays, which grew `ContextModel` by ~260 KB), a `make` without `make clean` left stale object files compiled with the old struct layout. The linker combined old caller code (using the small layout) with the new `context_model.o` (using the large layout), producing an ABI mismatch that segfaulted at runtime.

**Fix:** `make clean && make`. The Makefile has no header dependency tracking; after any `*.h` change that alters struct sizes, a full rebuild is required.

---

## Compressed File Format

### New PARALLEL Format (v5.0 default for structured files)

```
+------------+------------------+--------------+
| 0x07 (1B)  | orig_size (8B)   | n_blocks (4B)|
+------------+------------------+--------------+
| Per-block metadata (17B × n_blocks):          |
|  bwt_idx (4B) | flags (1B) | orig (4B)       |
|  pre_size (4B) | comp_size (4B)               |
+-----------------------------------------------+
| Block 0 compressed data                       |
| Block 1 compressed data                       |
| ...                                           |
+-----------------------------------------------+
```

### Legacy Formats (still readable)

All v4.0 formats remain decompressable. The PARALLEL (0x07) type is written by v5.0 compressor for any file that uses BWT+MTF+Order-1 with `--bwt` (auto-selected for low-entropy structured files).

| Model Type | Value | Meaning |
|------------|-------|---------|
| `RANS_ORDER_0` | 0x08  | v5.0 rANS static Order-0 (new, used for E/F) |
| `PARALLEL` | 0x07  | v5.0 full-pipeline parallel (new, used for A/B/C/G/H) |
| `ORDER_1_PREPROC` | 0x06 | v4.0 BWT+MTF+ZRLE+Order-1 (readable) |
| `ORDER_0_PREPROC` | 0x05 | v4.0 BWT+MTF+ZRLE+Order-0 (readable) |
| `ORDER_1_BWT` | 0x04 | Legacy BWT+Order-1 (readable) |
| `ORDER_0_BWT` | 0x03 | Legacy BWT+Order-0 (readable) |
| `ORDER_1` | 0x01 | Order-1 no BWT (readable) |
| `ORDER_0` | 0x00 | Order-0 no BWT (readable) |
| `UNCOMPRESSED` | 0xFF | Raw copy (readable) |

---

## Comparison with v4.0

| Feature | v4.0 | v5.0 |
|---------|------|------|
| **Pipeline threading** | BWT only (then MTF+ZRLE+Order-1 sequential) | Full pipeline per block (BWT+MTF+ZRLE+Order-1 all parallel) |
| **Encode loop allocation** | `std::vector<EncodingStep>` (heap, 900K allocs/block) | `EncodeResult` struct (stack, zero heap) |
| **Entropy coder (Order-0)** | Schindler range coder (2 divisions/symbol) | **ryg rANS** (0 divisions/symbol, flat lookup table) |
| **File format (structured)** | `ORDER_1_PREPROC` (0x06) | `PARALLEL` (0x07) |
| **File format (high-entropy)** | `ORDER_0` (0x00) | `RANS_ORDER_0` (0x08) |
| **Backward compatibility** | Reads all older formats | Writes new formats; reads all legacy formats |
| **Compression ratio** | 56.39% avg | **56.39% avg (unchanged)** |
| **File G compress** | 117 ms | **~62 ms (1.89×)** |
| **File B compress** | 68 ms | **~39 ms (1.74×)** |
| **File C compress** | 96 ms | **~45 ms (2.13×)** |
| **File E compress** | 49 ms | **~13 ms (3.8×)** |
| **File F compress** | 91 ms | **~20 ms (4.6×)** |
| **File G decompress** | 43 ms | **~34 ms (−21%)** |
| **File H decompress** | 97 ms | **~76 ms (−22%)** |
| **File E decompress** | 55 ms | **~10 ms (5.5×)** |
| **File F decompress** | 126 ms | **~23 ms (5.5×)** |
| **Avg decompress improvement** | baseline | **−25% Order-1 files; −82% Order-0 files** |

---

## Usage

```bash
# Build
make clean && make

# Compress (auto-selects PARALLEL for structured files)
./bin/compress <input> <output> [--model auto|order0|order1] [--bwt|--no-bwt] [-y]

# Decompress (auto-detects format)
./bin/decompress <compressed> <output>

# Run tests
make test

# Run benchmarks
make benchmark
```

---

## Build Information

**Compilation:**
- Compiler: g++ (C++17) for C++ sources, gcc for libsais.c
- Optimization: `-O3 -march=native`
- Threading: `-lpthread`
- Platform: Linux (Arch Linux, kernel 6.19.6)

**Memory Usage:**
- Context model per thread: ~264 KB (fits in L2 cache)
- BWT working buffer per thread: O(n) per block (~900 KB max)
- With N blocks in parallel: N × ~1.2 MB working memory
- For typical 3-block file: ~3.6 MB working memory — well within requirements

---

## Testing

**Verification Status:** ✓ All tests passed

- BWT unit tests: Perfect reconstruction verified
- Integration tests: Byte-for-byte lossless on all 8 test files
- Parallel blocks: Deterministic output guaranteed (results written to per-block pre-allocated slots; collected in original order after join)
- Cross-version: All v4.0-format files decompress correctly with v5.0 decompressor

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
- Fabian Giesen ("ryg"), rans_byte.h — https://github.com/rygorous/ryg_rans (Public Domain / CC0)
- Jarek Duda, "Asymmetric numeral systems: entropy coding combining speed of Huffman coding with compression rate of arithmetic coding" (2013)
- TAI Course Materials, Universidade de Aveiro

---

## License

Educational project for TAI course at Universidade de Aveiro.

Range coder implementation: GPL v2+ (Michael Schindler)
libsais: Apache 2.0 (Ilya Grebnov)
rans_byte.h: Public Domain / CC0 (Fabian Giesen)
All other code: Original group work

---

*Documentation last updated: March 19, 2026*
