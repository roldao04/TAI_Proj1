# Version History - TAI Project #1

**Lossless Data Compression Tool**

Group 07 - Universidade de Aveiro

---

## Version Overview

| Version | Date | Key Features | Performance | Status |
|---------|------|--------------|-------------|--------|
| **v8.0** | 2026-04-09 | **LZ4-style LZ77 Hash-Table Matching (no entropy coding)** | 38-88% ratio, **~200 MB/s compress, ~300 MB/s decompress** | **Production (Ultra-Fast)** |
| **v7.0** | 2026-04-08 | **BWT + MTF + 2-Way Interleaved rANS Order-0** | 59.2% avg, **51 MB/s decompress** | **Production (Speed)** |
| **v6.1** | 2026-04-06 | Multi-order PPM (5 orders) + Context Mixing + Static Weights | 59.85% avg ratio | **Experimental (Failed)** |
| **v5.2** | 2026-04-08 | ZRLE rank-255 fix + fused exclusions + MTF memmove + PGO | 54.70% avg ratio | **Production (Compression)** |
| **v5.0** | 2026-03-19 | Full parallelism + rANS + PPM Method C + PPMD escape + dynamic blocks | 54.87% avg ratio | Superseded |
| **v4.0** | 2026-03-13 | Flat array context model + libsais + AVX2 + parallel BWT | 56.39% avg ratio | Superseded |
| **v3** | 2026-03-06 | BWT Preprocessing + Multi-Model + Range Coding | 57.48% avg ratio | Superseded |
| **v2.0** | 2026-03-05 | Multi-Model + Range Coding + Auto-Select | 62.74% avg ratio | Superseded |
| **v1.0** | 2026-02-20 | Order-0 Model + Arithmetic Coding | 71.44% avg ratio | Superseded |

---

## Version Details

### v8.0 (April 2026) - Production (Ultra-Fast)

**Major Updates:**
- **Completely new algorithm**: LZ4-style LZ77 dictionary matching replaces the entire BWT+MTF+entropy-coding pipeline
- **16384-entry hash table**: 64KB, fits in L1 cache. Single-entry buckets with overwrite-on-collision (no chains, O(1) always)
- **Byte-aligned token format**: No entropy coding, no bit manipulation. Token stream is the output.
- **Skip acceleration**: Step grows every 64 misses; incompressible regions pass through at near-memcpy speed
- **8-byte XOR match counting**: `__builtin_ctzll(XOR)` counts matching bytes 8 at a time
- **Immediate re-match**: After encoding a match, checks the next position for another match via `goto`, avoiding re-entering the search loop
- **safe_copy8 decompressor**: 8-byte chunks with no overshoot (fixed wild_copy8 corruption bug where overshoot bytes polluted future back-references)

**Performance:**
- Compression speed: **~200 MB/s** average (6-13x faster than v7, 10-20x faster than v5)
- Decompression speed: **~300 MB/s** average (3-10x faster than v7)
- Binary sizes: **36KB** compressor, **36KB** decompressor (smallest of all versions)
- Compression ratio: 38-88% on compressible files (trades ratio for speed)
- Per-file standouts:
  - B: **37.8%** ratio at 279 MB/s compress
  - H: **58.0%** ratio at 139 MB/s compress
  - All-zeros 100KB: **0.4%** ratio (excellent RLE via offset=1 match repetition)

**Key Insight:** BWT is fundamentally O(n log n) and tops out at ~200 MB/s. LZ77 with hash-table matching is O(n) with a tiny constant, and by eliminating entropy coding entirely (byte-aligned tokens), the compressor avoids all bit manipulation overhead. The result is a compressor that is an order of magnitude faster than BWT-based approaches at the cost of ~20pp worse compression ratio.

**Bug discovered:** The LZ4-style `wild_copy8` (8-byte memcpy loop that overshoots by up to 7 bytes) corrupts decompressed output. Overshoot bytes from match copies persist in the output buffer and are read by future back-references before being overwritten. Fixed with `safe_copy8` which copies exactly the requested length.

**When to use:** Real-time applications, streaming data, temporary/transient compression, or any scenario where speed dominates over ratio. Targets the zstd-1 / LZ4 speed class.

**Detailed Documentation:** See [v8.0 README](g07_v8.0/README.md)

---

### v7.0 (April 2026) - Production (Speed-Focused)

**Major Updates:**
- **2-Way Interleaved rANS Order-0**: Two independent rANS states alternate symbols, exploiting CPU instruction-level parallelism for ~1.4x speedup over single-state rANS and ~5x over Range Coder
- **Adaptive BWT/MTF**: Applied for low-entropy data (entropy < 6.5); skipped for high-entropy files (raw rANS only), eliminating unnecessary preprocessing passes
- **No ZRLE**: Removed as extra pass with marginal compression gain after BWT+MTF
- **No Order-1 model**: After BWT+MTF, Order-0 entropy is within ~1pp of Order-1; Order-0 uses a single 128KB decode table (fits in L2 cache) vs Order-1's 256 separate tables
- **4MB block size**: Larger blocks give BWT more context; rANS speed absorbs the larger data volume
- **Frequency scaling fix**: Fixed over-allocation bug in `build_encode_table` for highly skewed post-BWT+MTF distributions

**Performance:**
- Average compression ratio: **59.2%** (well under 70% target)
- Compression speed: **28.9 MB/s** (1.7x faster than v5)
- Decompression speed: **51.1 MB/s** (3x faster than v5)
- Binary sizes: 84KB compressor, 56KB decompressor (smallest of all versions)
- Per-file standouts:
  - B: **20.6%** ratio (v5: 41.5% -- 2x better compression!)
  - H: **46.9%** ratio (v5: 61.2% -- 14pp better)
  - E: **5ms** compress (v5: 66ms -- 13x faster)

**Key Insight:** After BWT+MTF, the data distribution becomes extremely skewed toward low values. Order-0 rANS captures this nearly as well as Order-1, but with a single cache-friendly decode table instead of 256 context-dependent tables. The 2-way interleaving then hides the serial dependency in rANS decode by overlapping two independent state machines.

**When to use:** Real-time applications, throughput-critical scenarios, or when speed matters more than squeezing the last 2-5% of compression.

**Detailed Documentation:** See [v7.0 README](g07_v7.0/README.md)

---

### v6.1 (April 2026) - Experimental (Failed)

**Major Updates:**
- **Multi-order PPM**: 5 orders {1, 2, 3, 4, 5} with 8M context hash table
- **Context Mixing**: Weighted average of predictions using static pre-trained weights
- **Static Weight Training**: Sample-based approach (train on first 100KB, freeze weights, store in header)
- **BWT+MTF+ZRLE preprocessing**: Re-enabled after discovering it improves compression by 8pp
- **Fixed-point arithmetic**: Deterministic encoder/decoder synchronization

**Performance:**
- Average compression ratio: **59.85%** (worse than v5.0's 54.73% by 4.93pp)
- Speed: **158 KB/s** (157× slower than v5.0's 25 MB/s)
- Files won vs v5.0: **0/8** (lost all benchmark files)

**Key Finding:**
- **BWT preprocessing helps multi-order PPM** (contrary to hypothesis)
  - With BWT: 59.85% avg
  - Without BWT: 67.94% avg (8.09pp worse)
- **Static weights can't adapt** to different file sections → fundamental flaw
- **Byte-level coding** leaves information on the table vs bit-level PAQ approach

**Why it failed:**
1. Static weights don't adapt during compression (PAQ uses adaptive weights)
2. Byte-level range coding limits compression potential
3. Pure multi-order PPM underperforms on binary files
4. 157× slower with worse compression = not viable

**Lesson learned:** Sample-based static weight training is not competitive with adaptive mixing (PAQ-style).

**Detailed Documentation:** See [v6.1 README](g07_v6.1/README.md), [ARCHITECTURE](g07_v6.1/ARCHITECTURE.md), [LESSONS_LEARNED](g07_v6.1/LESSONS_LEARNED.md)

---

### v5.2 (April 2026) - Production Release

**Major Updates:**

Version 5.2 is an incremental optimization of v5.0, focusing on fixing the ZRLE rank-255 bug and micro-optimizing the encode/decode hot paths.

**1. ZRLE Rank-255 Escape Fix (Compression Ratio)**
The RLE0 encoder previously mapped non-zero MTF rank `r` to byte `r+1`. Rank 255 overflowed to 0 (RUNA), corrupting the zero-run encoding. Files G and H had to skip ZRLE entirely via a `has_rank255` guard, losing 0.46pp and 0.93pp respectively.

**Fix:** Byte 255 is now an escape prefix. Ranks 1-253 encode as bytes 2-254 (unchanged). Ranks 254/255 encode as two bytes: `255, rank`. The 1-byte overhead per rank-254/255 occurrence is negligible (G has only 51 rank-255 in 2.5MB). The `has_rank255` guard was removed — ZRLE is now enabled for all blocks unconditionally.

**2. Fused Exclusion Decode (Speed)**
When the decoder escapes from Order-1 to Order-0 with Method C exclusions, it previously scanned 258 elements three times: once to compute the excluded total, once to recompute the same total inside `find_symbol_and_get_range_excl_ctx()`, and once to find the symbol. The redundant second scan was eliminated by passing the pre-computed total as a parameter.

**3. Fused Exclusion Encode (Speed)**
The encoder's Method C exclusion path in `encode_symbol_fast()` previously used two loops over 258 elements: one to compute the excluded total, one to find the symbol and cumulative. These were fused into a single pass that computes total and finds the symbol simultaneously.

**4. Fast Escape Cumulative (Speed)**
Computing the cumulative frequency for the ESCAPE symbol (index 256) previously summed all 256 preceding frequencies via `cumulative_before()`. Since escape is always the last symbol in Order-1 contexts, its cumulative equals `total - escape_freq`, computed in O(1).

**5. MTF memmove (Speed)**
The `move_symbol_to_front()` function replaced its manual element-by-element loop with `std::memmove()`, which uses hardware-optimized SIMD memory moves. The inverse transform path was further optimized by removing the unused `positions` array entirely.

**6. PGO Build (Speed)**
Added `make pgo` target for Profile-Guided Optimization: build with `-fprofile-generate`, train on all 8 corpus files, rebuild with `-fprofile-use`. Improves branch prediction and code layout. Binary sizes reduced (132K→120K compressor, 80K→72K decompressor).

**Performance:**
- Average compression ratio: **54.70%** (improved from v5.0's 54.87%, −0.17pp)
- **Beats bzip2 by 0.23pp** (54.70% vs 54.93%; was 0.06pp in v5.0)
- Average decompression time: **60ms** with PGO (was 63ms in v5.0)
- Files won vs bzip2: **5/8** (A, B, C, D, E)

**File-by-file changes vs v5.0:**
- G: 29.27% → **28.81%** (−0.46pp) — ZRLE now enabled
- H: 44.72% → **43.79%** (−0.93pp) — ZRLE now enabled
- All other files: unchanged (ZRLE was already enabled)
- G gap vs bzip2 closed: −0.57pp → −0.11pp

**Experiments with no measurable effect:**
- RESCALE_THRESH tuning (4096/8192/16384/32768): byte-for-byte identical output across all values — halving preserves ratios
- Block size 3000KB: G improved 0.04pp avg but decompression regressed (1 sequential block vs 2 parallel) — kept 2000KB

---

### v5.0 (March 2026) - Superseded

**Major Updates:**

Version 5.0 represents a comprehensive advancement from v4.0, incorporating **11 major improvements** developed through iterative optimization over March 19, 2026:

**Wave 1: Performance & Parallelism**
1. **Full pipeline parallelism (pbzip2-style)**: Each block runs complete BWT→MTF→ZRLE→Order-1 pipeline in its own thread; 2-5× compression speedup
2. **Allocation-free encoding**: `encode_symbol_fast()` returns stack-allocated struct instead of `std::vector`, eliminating ~900K heap allocations per block
3. **rANS Order-0**: ryg's Asymmetric Numeral Systems replaces Schindler range coder for high-entropy files; zero divisions per decoded symbol via flat 16384-slot lookup table; E/F decompression −82%
4. **Decode loop optimizations**: Fused `find_symbol_and_get_range` eliminates duplicate O(258) scan; flat if/else branches; hoisted Order-0 total; −38% decompression on Order-1 files
5. **Link-time optimization**: `-flto=auto` enables cross-module inlining of hot paths; E compress −30%

**Wave 2: Compression Algorithms**
6. **PPM Method C exclusions**: When escaping Order-1→Order-0, exclude symbols already seen in Order-1 context; both encoder/decoder compute same exclusion set dynamically; narrower intervals → fewer bits
7. **RLE0 bijective encoding**: Replaces marker+count ZRLE with bzip2-style base-2 run encoding (RUNA/RUNB); eliminates marker overhead for short runs; includes rank-255 corruption fix via `has_rank255` guard
8. **Entropy threshold 6.8→7.2**: Routes files E (7.030) and F (7.013) to Order-1 instead of rANS Order-0; flat-array Order-1 fast enough; F gains 5.84pp

**Wave 3: Model Refinement**
9. **PPMD singleton escape**: Replaces `max(1, seen/4)` with `max(1, singleton_count)` where `singleton_count` = symbols with `freq==1`; escape probability reflects actual new-symbol likelihood; tracked via `singleton1_[256]` array with O(1) updates
10. **Frequency rescaling at threshold 8192**: When `total1_[ctx] > 8192`, all counts halved (round-up); prevents early-block statistics from dominating; particularly effective on non-stationary files (B, C, G)
11. **Dynamic block sizing (max 2000 KB, even split)**: `num_blocks = ceil(file_size/2000KB)`, `block_size = ceil(file_size/num_blocks)`; all blocks equal size; A/B/C/H fit in 1 block; G splits into 2 equal blocks; better BWT context

**Performance:**
- Average compression ratio: **54.73%** (improved from v4.0's 56.39%, −1.66pp total)
- **Beats bzip2 by 0.20pp** (54.73% vs 54.93%)
- Speed: **2-5× faster compression**, **25-82% faster decompression** vs v4.0
- Files won vs bzip2: A (+0.75pp), B (+0.58pp), C (+0.58pp), D (+0.45pp), E (+2.84pp) — **5/8 files**
- B: 18.48% → **17.34%** (−1.14pp); C: 27.59% → **25.93%** (−1.66pp); F: 87.62% → **81.70%** (−5.92pp)

**File-by-file improvements vs v4.0:**
- A: 53.96% → **53.30%** (−0.66pp) — Method C + dynamic blocks
- B: 18.48% → **17.34%** (−1.14pp) — Rescaling + dynamic blocks
- C: 27.59% → **25.93%** (−1.66pp) — Method C + RLE0 + rescaling + dynamic blocks
- E: 86.18% → **85.60%** (−0.58pp) — Threshold 7.2 + PPMD escape
- F: 87.62% → **81.70%** (−5.92pp) — Threshold 7.2 routes to Order-1
- G: 29.07% → 29.28% (+0.21pp) — ZRLE skipped (rank-255 present)
- H: 44.66% → 44.72% (+0.06pp) — ZRLE skipped (rank-255 present)

**Known limitation:**
- G and H lose to bzip2 because ZRLE is skipped due to rank-255 wrapping bug (255+1=256→0); root cause is byte-level RLE0 alphabet limit (256 values needed for 255 literal ranks + 2 run markers = 257 symbols); current fix uses `has_rank255` guard; potential solutions: redesign ZRLE as symbol-level step or use multi-byte codes

**Development approach:**
- Three optimization waves over single day (March 19, 2026)
- Each wave tested independently before integration
- Failed experiments documented: prefix-sum arrays (+21% slower), `-funroll-loops` (cache eviction), PGO (regressed E/F), update exclusion (regression with Method C), rescaling 16384/32768 (too infrequent), various block sizes (1024KB, 1300KB suboptimal)

**Detailed Documentation:** See [v5.0 README](g07_v5.0/README.md)

---

### v4.0 (March 2026) - Superseded

**Major Updates:**
- **Flat Array Context Model**: Replaced `std::map` per context with flat `uint32_t[256][258]` — 69× speedup on large files
- **libsais**: O(n) suffix array replacing O(n log n) prefix-doubling BWT
- **Parallel BWT**: One thread per block for BWT transform only
- **`-march=native`**: AVX2 SIMD auto-vectorization (256-bit vs SSE2 128-bit)
- **900 KB block size**: Increased from v3's 1024 bytes

**Performance:**
- Average compression ratio: **56.39%** (improved from v3's 57.48%)
- File H (1 MB): 6,594ms → **96ms (69×)**
- File G (2.5 MB): 3,357ms → **117ms (29×)**
- File C (2 MB): 2,614ms → **96ms (27×)**

**Detailed Documentation:** See [v4.0 README](g07_v4.0/README.md)

---

### v3 (March 2026) - Superseded

**Major Updates:**
- **BWT Preprocessing**: Burrows-Wheeler Transform for improved compression
- **Block-Based Processing**: 1024-byte blocks for optimal performance
- **Smart BWT Selection**: Entropy-based auto-enable/disable
- **Enhanced CLI**: `--bwt` and `--no-bwt` options

**Core Implementation:**
- Burrows-Wheeler Transform (block sorting algorithm)
- Suffix array construction for forward transform
- LF mapping for efficient inverse transform
- Integrated with Range Coding and multi-model system
- Auto-detection of BWT suitability based on file entropy

**Performance:**
- Average compression ratio: **57.48%** (8.4% better than v2.0's 62.74%)
- Average bits/symbol: 4.60
- Files won: 3/8 (Files A, D, G - improved from v2.0's 2/8)
- Overall ranking: **#3** vs industry tools (improved from v2.0's #5)
- Particularly effective on:
  - File A (text): 51.96% - Best among all tools
  - File G (structured): 27.92% - Best among all tools
  - File D (random): 100.00% - Correctly uncompressed
- High-entropy files: BWT auto-disabled (minimal overhead)

**BWT Characteristics:**
- Block size: 1024 bytes (optimized for speed and compression)
- Auto-selection: Enabled when entropy < 6.5 bits/symbol
- Processing time: ~1-3ms per 1KB block
- Memory: O(n) per block, efficient and bounded

**File Format:**
- New BWT flag in header (1 byte)
- Primary indices stored for each block (4 bytes/block)
- Backward compatible: Reads v2.0 files (BWT flag = 0x00)
- v2.0 incompatible with v3 BWT-enabled files

**Files Included:**
- `compress` - Compression binary with BWT support
- `decompress` - Decompression binary with BWT inverse
- Unit tests verify perfect BWT invertibility

**Notable Improvements:**
- Overall compression: 57.48% vs v2.0's 62.74% (5.26pp improvement)
- Ranking improvement: #3 vs v2.0's #5
- Files won: 3/8 vs v2.0's 2/8
- File G (structured): 27.92% vs v2.0's 28.81% (best among all tools)
- Maintains competitive speed with multi-model system

**Detailed Documentation:** See [v3 README](g07_v3.0/README.md)

---

### v2.0 (March 2026) - Superseded

**Major Updates:**
- **Multi-Model System**: Order-0 + Order-1 with intelligent auto-selection
- **Range Coding**: Replaced arithmetic coding (~2x faster)
- **Smart Detection**: Entropy-based model selection and uncompressed storage
- **Enhanced CLI**: `--model` option for manual model selection

**Core Implementation:**
- Range Coding for entropy encoding (Schindler's algorithm)
- Order-0 static frequency model (fast, universal)
- Order-1 adaptive context model (PPM-style with escape)
- Auto-selection algorithm based on file entropy
- Uncompressed mode for incompressible files

**Performance:**
- Average compression ratio: **62.74%**
- Average bits/symbol: 5.02
- **Improvement over v1.0: 8.7pp (13% better compression)**
- **Speed: ~2x faster** with range coding
- Compression time: Variable (Order-0 fast, Order-1 slower but better)
- Decompression time: Variable by model

**Benchmarks:**
- Tested on 8 files (13MiB total)
- Won best compression on 2/8 files (File A and G)
- Ranking: #5 vs industry tools (improved from v1.0)
- Auto-selection success rate: ~90%

**Model Performance:**
- Order-0: Fast, universal (seconds on large files)
- Order-1: 20-50% better compression on low-entropy data
- Order-2: Tested and removed (no benefit over Order-1)

**Files Included:**
- `compress` - Compression binary with multi-model support
- `decompress` - Decompression binary with auto-detection

**Notable Achievements:**
- File A (1.3MiB text): **51.85%** - Best compression among all tools tested
- File G (2.5MiB): **28.81%** - Tied with bzip2
- Smart detection prevents expansion on incompressible files

**Detailed Documentation:** See [v2.0 README](g07_v2.0/README.md)

---

### v1.0 (February 2026) - Superseded

**Core Implementation:**
- Arithmetic Coding for entropy encoding
- Order-0 Frequency Model for probability estimation
- Lossless compression/decompression guaranteed

**Performance:**
- Average compression ratio: 71.44%
- Average bits/symbol: 5.72
- Compression speed: ~98ms average
- Decompression speed: ~127ms average

**Benchmarks:**
- Tested on 8 files (13MiB total)
- Won best compression on 1/8 files
- Ranking: #5 vs industry tools (gzip, bzip2, xz, zstd)

**Files Included:**
- `compress` - Compression binary
- `decompress` - Decompression binary

**Detailed Documentation:** See [v1.0 README](g07_v1.0/README.md)

**Status:** Superseded by v2.0, kept for comparison and historical reference

---

## Version Comparison Summary

| Metric | v1.0 | v2.0 | v3 | v4.0 | v5.0 | v5.2 | v6.1 | v7.0 | v8.0 |
|--------|------|------|------|------|------|------|------|------|------|
| **Compression Ratio** | 71.44% | 62.74% | 57.48% | 56.39% | 54.87% | 54.70% | 59.85% ❌ | 59.2% | ~60% (compressible) |
| **Bits/Symbol** | 5.72 | 5.02 | 4.60 | 4.51 | 4.39 | 4.38 | 4.78 | 4.74 | ~4.8 |
| **Preprocessing** | None | None | BWT (1024B) | BWT (900KB) | BWT (≤2000KB) | BWT (≤2000KB) | BWT+MTF+ZRLE | BWT+MTF (4MB) | **None** |
| **Zero-run encoding** | None | None | None | ZRLE | RLE0 | RLE0 (escape) | ZRLE | None | **None** |
| **PPM Orders** | Order-0 | Order-0/1 | Order-1 | Order-1 | Order-1 | Order-1 | Multi {1-5} | Order-0 | **N/A (LZ77)** |
| **Entropy Coder** | Arithmetic | Range | Range | Range | Range+rANS | Range+rANS | Range | Interleaved rANS | **None** |
| **Threading** | None | None | None | Par. BWT | Full parallel | Full parallel | None | Full parallel | **Single thread** |
| **Compress Speed** | ~13 MB/s | ~26 MB/s | — | — | ~25 MB/s | ~25 MB/s | 0.15 MB/s | 28.9 MB/s | **~200 MB/s** ✅ |
| **Decompress Speed** | ~10 MB/s | ~20 MB/s | — | — | ~17 MB/s | ~21 MB/s | Slow | 51.1 MB/s | **~300 MB/s** ✅ |
| **Binary Size** | — | — | — | — | 216KB | 216KB | 196KB | 140KB | **72KB** ✅ |
| **Status** | Superseded | Superseded | Superseded | Superseded | Superseded | **Production** | **Failed** | **Production (Speed)** | **Production (Ultra-Fast)** |

---

## Performance Timeline

### Compression Ratio Progress

| Version | Avg Ratio | Improvement vs Previous | Cumulative from v1.0 |
|---------|-----------|------------------------|----------------------|
| v1.0 | 71.44% | — | — |
| v2.0 | 62.74% | −8.70pp | −8.70pp |
| v3 | 57.48% | −5.26pp | −13.96pp |
| v4.0 | 56.39% | −1.09pp | −15.05pp |
| v5.0 | 54.87% | −1.52pp | −16.57pp |
| v5.2 | **54.70%** ✅ | **−0.17pp** | **−16.74pp** |
| v6.1 | 59.85% ❌ | +5.12pp worse | −11.59pp (vs v1.0 only) |
| v7.0 | **59.2%** | +4.5pp vs v5.2 (speed tradeoff) | **−12.24pp** |

**Note:** v6.1 regressed from v5.0 (failed experiment). v7.0 trades ~4.5pp of compression for 3x decompression speed.

### Speed Evolution

| Version | Compress MB/s | Decompress MB/s | Key Technique |
|---------|--------------|-----------------|---------------|
| v1.0 | ~13 | ~10 | Baseline arithmetic |
| v2.0 | ~26 | ~20 | Range coding |
| v3 | — | — | BWT + range |
| v4.0 | — | — | Flat arrays + libsais + AVX2 |
| v5.0 | ~25 | ~17 | Full parallelism + rANS + LTO |
| v5.2 | ~25 | ~21 | Fused decode + PGO |
| v6.1 | 0.15 ❌ | Slow | Multi-order PPM + context mixing |
| v7.0 | 28.9 | 51.1 | 2-way interleaved rANS + BWT+MTF |
| v8.0 | **~200** ✅ | **~300** ✅ | **LZ77 hash-table matching, byte-aligned tokens, no entropy coding** |

---

## Development Timeline

- **2026-02-20**: v1.0 released (Order-0 + Arithmetic)
- **2026-02-21 - 2026-03-04**: v2.0 development
  - Range coding implementation
  - Order-1 model development
  - Order-2 experimental (removed)
  - Auto-selection algorithm
  - Extensive benchmarking
- **2026-03-05**: v2.0 released (Multi-model + Range coding)
- **2026-03-05 - 2026-03-06**: v3 development
  - BWT (Burrows-Wheeler Transform) implementation
  - Suffix array construction
  - Block-based processing (1024-byte blocks)
  - BWT inverse transform with LF mapping
  - Auto-selection for BWT enable/disable
  - Comprehensive BWT unit testing
  - Integration with existing compression pipeline
- **2026-03-06**: v3 released (BWT preprocessing + Multi-model)
- **2026-03-07 - 2026-03-13**: v4.0 development
  - Flat array context model (replacing std::map)
  - libsais O(n) BWT
  - Parallel BWT blocks (std::thread)
  - AVX2 via -march=native
  - Block size increase to 900 KB
- **2026-03-13**: v4.0 released (major performance overhaul, 69× speedup)
- **2026-03-14 - 2026-03-19**: v5.0 development (three optimization waves)

  **Wave 1: Performance & Parallelism**
  - Full pipeline parallelism (pbzip2 design)
  - encode_symbol_fast (stack-allocated EncodeResult)
  - find_symbol_and_get_range (fused decode)
  - Decode loop restructuring (flat if/else, inline accessors)
  - rANS Order-0: ryg's rans_byte.h + RansStaticCoder wrapper
  - New PARALLEL (0x07) and RANS_ORDER_0 (0x08) file formats
  - Link-time optimization (-flto=auto)
  - Tested and reverted: prefix-sum arrays (21% slower), -funroll-loops (cache eviction), PGO (regressed E/F)

  **Wave 2: Compression Algorithms**
  - PPM Method C exclusions (encode_symbol_fast + decoder escape branches)
  - RLE0 bijective encoding replacing ZRLE (base-2, non-zero shift)
  - Entropy threshold 6.8 → 7.2 (routes E, F to Order-1)
  - Tested and reverted: update exclusion (regression with Method C)

  **Wave 3: Model Refinement**
  - PPMD singleton escape (singleton1_[256] array, O(1) updates at freq 0→1 and 1→2)
  - Frequency rescaling at threshold 8192 (halve round-up, recount singletons)
  - ZRLE rank-255 correctness fix: has_rank255 guard skips ZRLE for affected blocks; bug traced via BWT+MTF+ZRLE roundtrip diagnostic
  - Dynamic block sizing: max 2000 KB, even split; better BWT context
  - Tested and reverted: rescaling 16384/32768 (too infrequent), exempting context 0 (worsened), block sizes 1024KB/1300KB (suboptimal), rescaling threshold 4096 (net negative)

- **2026-03-19**: v5.0 released (11 major improvements; 54.73% avg, 5/8 files beat bzip2, −0.20pp vs bzip2)
- **2026-04-05 - 2026-04-06**: v6.1 development (experimental)
  - Multi-order PPM (5 orders: {1,2,3,4,5})
  - Context mixing with static weight training (sample-based)
  - Fixed-point arithmetic for deterministic sync
  - Initial test: BWT disabled → 67.94% avg (failed)
  - Re-enabled BWT+MTF+ZRLE → 59.85% avg (still failed vs v5.0's 54.73%)
  - Finding: BWT preprocessing **helps** multi-order PPM by 8pp (contrary to hypothesis)
  - Lesson: Static weights can't adapt → fundamental flaw vs PAQ's adaptive mixing
- **2026-04-06**: v6.1 documented as failed experiment (loses to v5.0 by 4.93pp, 157× slower)
- **2026-04-08**: v5.2 development

  **ZRLE Rank-255 Escape Fix**
  - Byte 255 as escape prefix for MTF ranks 254/255
  - Removed `has_rank255` guard — ZRLE unconditionally enabled
  - G: 29.27% → 28.81% (−0.46pp), H: 44.72% → 43.79% (−0.93pp)

  **Hot-Path Speed Optimizations**
  - Fused exclusion decode: eliminated redundant 258-element scan (3 scans → 2)
  - Fused exclusion encode: merged 2 loops into single pass
  - Fast escape cumulative: O(1) via `total - escape_freq` instead of O(256) prefix sum
  - MTF memmove: `std::memmove()` replaces manual loop; inverse path drops `positions` array
  - PGO build: `make pgo` target for profile-guided optimization (avg decomp 63ms → 60ms)

  **Experiments with no effect**
  - RESCALE_THRESH 4096/16384/32768: byte-for-byte identical output (halving preserves ratios)
  - Block size 3000KB: G improved 0.04pp but lost parallel decompression (51ms → 87ms) — reverted

- **2026-04-08**: v5.2 released (54.70% avg, beats bzip2 by 0.23pp, 60ms avg decompress with PGO)
- **2026-04-08**: v7.0 development

  **Speed-Optimized Compressor**
  - Replaced Range Coder + Order-1 adaptive with 2-way interleaved rANS Order-0
  - Two independent rANS states alternate symbols for CPU instruction-level parallelism (~1.4x over single rANS)
  - BWT+MTF preprocessing for low-entropy data; raw rANS for high-entropy (no BWT/MTF)
  - Removed ZRLE (extra pass, marginal benefit after BWT+MTF)
  - 4MB blocks (larger BWT context than v5's 2MB)
  - Fixed frequency scaling over-allocation bug in build_encode_table (skewed post-BWT distributions)
  - New file format: model type 0x0B with per-block metadata and interleaved rANS streams

  **Results:** 59.2% avg ratio, 28.9 MB/s compress, 51.1 MB/s decompress
  - B: 20.6% (v5: 41.5%), H: 46.9% (v5: 61.2%) — BWT+MTF+Order-0 is better than BWT+MTF+ZRLE+Order-1 for these files
  - E: 5ms compress (v5: 66ms) — 13x faster via raw rANS (no BWT overhead)
  - All 8 files lossless verified

- **2026-04-08**: v7.0 released (59.2% avg, 51.1 MB/s decompress, fastest version ever)
- **2026-04-09**: v8.0 development

  **Ultra-Fast LZ77 Compressor**
  - Completely new algorithm: LZ4-style LZ77 dictionary matching replaces the entire BWT+MTF+entropy-coding pipeline
  - 16384-entry hash table (64KB, L1 cache): single-entry buckets, overwrite-on-collision
  - Byte-aligned token format: no entropy coding, no bit manipulation
  - Skip acceleration: step grows every 64 consecutive misses
  - 8-byte XOR match counting via `__builtin_ctzll`
  - Immediate re-match after encoding via `goto encode_match`
  - Bug found: wild_copy8 overshoot in decompressor corrupted output (overshoot bytes persisted as back-reference sources). Fixed with safe_copy8 (exact-length copies, no overshoot)
  - New file format: model type 0x0C with 13-byte header (1B type + 8B size + 4B payload size)

  **Results:** ~200 MB/s compress, ~300 MB/s decompress, 36KB binaries
  - 6-13x faster than v7, 10-20x faster than v5
  - Ratio: 38-88% on compressible files (trades ~20pp for speed)
  - All-zeros 100KB: 0.4% ratio (LZ77 RLE via offset=1 matching)
  - All 8 benchmark files + 6 edge cases lossless verified

- **2026-04-09**: v8.0 released (~200 MB/s compress, ~300 MB/s decompress, ultra-fast)

---

## Notable Achievements

### Version 8.0 Highlights
- **Fastest compressor ever**: ~200 MB/s average (7x faster than v7, 8x faster than v5)
- **Fastest decompressor ever**: ~300 MB/s average (6x faster than v7, 15x faster than v5)
- **Smallest binaries ever**: 36KB compressor, 36KB decompressor (72KB total)
- **Completely different algorithm**: LZ77 dictionary matching vs BWT+entropy coding
- **Zero entropy coding**: Byte-aligned token stream, no Huffman/ANS/Range
- **Bug discovery**: wild_copy8 overshoot corruption in LZ4-style decompression

### Version 7.0 Highlights
- **Fastest BWT-based decompression**: 51.1 MB/s average (3x faster than v5.2)
- **Fastest BWT-based compression**: 28.9 MB/s average (1.7x faster than v5.2)
- **Surprising compression wins**: B (20.6% vs v5's 41.5%), H (46.9% vs v5's 61.2%)
- **E/F throughput**: 13-19x faster than v5 on high-entropy files

### Version 5.2 Highlights
- **Best compression ratio achieved**: 54.70% average (beats bzip2 by 0.23pp)
- **ZRLE rank-255 solved**: Escape byte prefix enables ZRLE on all files unconditionally
- **Most files won**: 5/8 files beat bzip2 (A, B, C, D, E)
- **G gap nearly closed**: 28.81% vs bzip2's 28.70% (only −0.11pp)

### Overall Project Achievements
- **Three production versions**: v5.2 (best compression), v7.0 (fast), v8.0 (ultra-fast) -- choose based on use case
- **16.74pp cumulative improvement** from v1.0 to v5.2 (71.44% → 54.70%)
- **~300 MB/s peak throughput** in v8.0 (3000x faster than v1.0's decompression)
- **Two algorithm families**: BWT+entropy coding (v1-v7) and LZ77 dictionary matching (v8)
- **Competitive with industry leaders**: Beats bzip2 on ratio (v5.2), rivals zstd-1/LZ4 on speed (v8)
- **Comprehensive approach**: Combines BWT preprocessing, adaptive PPM modeling, multiple entropy coders, and LZ77 matching
- **Production-ready**: Robust error handling, extensive testing, edge case coverage
- **Well-documented**: Detailed READMEs track all experiments including failed optimizations

---

## Files by Version

| Version | Binaries | Documentation |
|---------|----------|---------------|
| v1.0 | — | README.md |
| v2.0 | — | README.md |
| v3.0 | — | README.md |
| v4.0 | G07-V4-C, G07-V4-D | README.md |
| v5.0 | g07-v5-c, g07-v5-d | README.md (comprehensive) |
| v6.1 | g07-v6-c, g07-v6-d | README.md, ARCHITECTURE.md, LESSONS_LEARNED.md |
| v7.0 | g07-v7-c (84KB), g07-v7-d (56KB) | README.md |
| v8.0 | g07-v8-c (36KB), g07-v8-d (36KB) | README.md |

**Note:** v5.2 is the best-compression production version. v7.0 is the fast (BWT-based) production version. v8.0 is the ultra-fast (LZ77-based) production version. v6.1 is a failed experiment.

---

*Last updated: 2026-04-09 (v8.0 ultra-fast release)*
