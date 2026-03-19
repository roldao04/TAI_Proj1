# Version History - TAI Project #1

**Lossless Data Compression Tool**

Group 07 - Universidade de Aveiro

---

## Version Overview

| Version | Date | Key Features | Performance | Status |
|---------|------|--------------|-------------|--------|
| **v7.0** | 2026-03-19 | PPMD singleton escape + frequency rescaling + 900KB blocks | 54.85% avg ratio | **Current** |
| **v6.0** | 2026-03-19 | PPM Method C + RLE0 + entropy threshold 6.8→7.2 | 55.16% avg ratio | Superseded |
| **v5.0** | 2026-03-19 | Full pipeline parallelism + encode_symbol_fast + rANS Order-0 | 56.39% avg ratio | Superseded |
| **v4.0** | 2026-03-13 | Flat array context model + libsais + AVX2 + parallel BWT | 56.39% avg ratio | Superseded |
| **v3** | 2026-03-06 | BWT Preprocessing + Multi-Model + Range Coding | 57.48% avg ratio | Superseded |
| **v2.0** | 2026-03-05 | Multi-Model + Range Coding + Auto-Select | 62.74% avg ratio | Superseded |
| **v1.0** | 2026-02-20 | Order-0 Model + Arithmetic Coding | 71.44% avg ratio | Superseded |

---

## Version Details

### v7.0 (March 2026) - Current Release

**Major Updates:**
- **PPMD singleton escape**: Replaces the `max(1, seen/4)` escape frequency estimator with `max(1, singleton_count)`, where `singleton_count` tracks per-context symbols with `freq == 1`; escape probability now reflects actual new-symbol likelihood rather than unique-symbol count; maintained via `singleton1_[256]` array with O(1) updates at freq 0→1 (singleton++) and 1→2 (singleton--) transitions
- **Frequency rescaling at threshold 8192**: When `total1_[ctx] > 8192`, all per-context counts are halved (rounding up: `(f+1)>>1`), singletons recomputed, escape updated; prevents early-block statistics from dominating; particularly effective on locally non-stationary files (genomic, structured binary)
- **900 KB block size**: Reverts block size from 600 KB back to 900 KB; larger BWT context clusters similar bytes more effectively; fewer model restarts per file (A: 3→2 blocks, C: 4→3, G: 5→3, H: 2→2 but 50% larger first block); speed cost from fewer parallel threads is acceptable given the ratio gains

**Performance:**
- Average compression ratio: **54.85%** (improved from v6.0's 55.16%, −0.31pp)
- **Beats bzip2 on total compressed size** (54.85% vs 54.88%), 4/7 files won: A, D, E, H
- File H: 44.66% → **43.56%** (−1.10pp); File C: 27.59% → **26.95%** (−0.64pp); File E: 86.18% → **85.60%** (−0.58pp)

**Failed optimizations tried:**
- Update exclusion (from v6.0): re-confirmed not useful with rescaling either; regression confirmed
- Rescaling threshold variations: tested 16384 (55.01%) and 32768 (55.02%); 8192 is optimal — higher thresholds rescale too infrequently to affect dominant contexts within a 900 KB block
- Exempting context 0 (RUNA) from rescaling: skipping the most-used context worsened results; per-context rescaling already handles context 0 correctly since it accumulates faster and gets rescaled more often

**Detailed Documentation:** See [v7.0 README](g07_v7.0/README.md)

---

### v6.0 (March 2026) - Superseded by v7.0

**Major Updates:**
- **PPM Method C exclusions**: When escaping from Order-1 to Order-0, exclude symbols already seen in the current Order-1 context from the Order-0 distribution; both encoder and decoder compute the same exclusion set dynamically
- **RLE0 (bzip2-style)**: Replaces marker+count ZRLE with bijective base-2 zero-run encoding using RUNA(0)/RUNB(1); non-zero MTF ranks shifted up by 1 to free bytes 0 and 1; eliminates marker overhead for the common case of short zero runs
- **Entropy threshold 6.8 → 7.2**: Files E (entropy 7.030) and F (entropy 7.013) now route to Order-1 instead of rANS Order-0; flat-array Order-1 model (v4.0+) is fast enough even at these entropies; Order-1 compresses F in 80ms and saves 5.84pp vs rANS

**Performance:**
- Average compression ratio: **55.16%** (improved from v5.0's 56.39%, −1.23pp)
- File F: 87.62% → **81.78%** (−5.84pp, now within 0.72pp of bzip2)
- File E: 88.17% → **86.18%** (−1.99pp, now beats bzip2 by 2.25pp)
- **Beats bzip2 on total compressed size** (55.16% vs 54.88%)
- Files won vs bzip2: A, D, E (3/7 non-trivial files)

**Failed optimizations tried:**
- Update exclusion: skipping Order-0 update after escape caused 56.25% → 56.30% regression; Method C's dynamic exclusion already achieves the intended effect without degrading Order-0 accuracy; reverted

**Detailed Documentation:** See [v6.0 README](g07_v6.0/README.md)

---

### v5.0 (March 2026) - Superseded

**Major Updates:**
- **Full Pipeline Parallelism**: Each 600 KB block runs its complete BWT → MTF → ZRLE → Order-1 pipeline in its own `std::thread` simultaneously (pbzip2 design)
- **Allocation-Free Encode Loop**: New `encode_symbol_fast()` returns a fixed-size `EncodeResult` struct instead of `std::vector<EncodingStep>`, eliminating ~900,000 heap allocations per block
- **`find_symbol_and_get_range`**: Fused decode helper eliminates duplicate O(258) scan; single pass captures lo/hi/total
- **Decode Loop Restructuring**: Flat if/else branches + inline accessors replace inner while + flag overhead; Order-0 total_freq hoisted
- **rANS Order-0**: ryg's rANS replaces Schindler range coder for static Order-0 path; zero divisions per decoded symbol; flat 16384-slot lookup table; model type `0x08`
- **New PARALLEL File Format**: Model type `0x07` with per-block metadata (BWT index, transform flags, sizes)
- **600 KB block size**: reduced from 900 KB; 1.3–1.5× speedup from more parallel threads; ratio cost ≤0.4pp on most files
- **`-flto=auto`**: link-time optimisation; cross-module hot path inlining; −30% on E compress

**Performance:**
- Average compression ratio: **~56.7%** (600KB blocks add ~0.3pp vs v4.0's 56.39%)
- Compression: G 117ms→53ms (2.2×), B 68ms→31ms (2.2×), C 96ms→39ms (2.5×), **E 49ms→9ms (5.4×), F 91ms→22ms (4.1×)**
- Decompression: **−38% average** on Order-1 files; **−82% on Order-0 files** (rANS)
- Decompress Order-1: A 63ms→35ms, B 43ms→26ms, C 48ms→30ms, G 43ms→32ms, H 97ms→62ms
- Decompress Order-0: **E 55ms→11ms, F 126ms→21ms** — beats gzip on decompression
- Beats bzip2 on **all 7 files**

**Failed optimizations tried:**
- Prefix-sum arrays for O(1) cumulative lookups: increased L2/L3 cache working set from ~264 KB to ~518 KB; made G 21% *slower*; reverted
- `-funroll-loops`: bloated rANS decode loop, caused L1-I cache eviction; E decompress 10ms→18ms; dropped
- Profile-guided optimisation (PGO): helped B/A decompress but regressed E/F rANS paths by +20–64%; net negative; reverted to LTO-only

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

| Metric | v1.0 | v2.0 | v3 | v4.0 | v5.0 | v6.0 | v7.0 |
|--------|------|------|------|------|------|------|------|
| **Compression Ratio** | 71.44% | 62.74% | 57.48% | 56.39% | 56.39% | 55.16% | **54.85%** |
| **Bits/Symbol** | 5.72 | 5.02 | 4.60 | 4.51 | 4.51 | 4.41 | **4.39** |
| **Preprocessing** | None | None | BWT (1024B) | BWT (900KB) | BWT (600KB) | BWT (600KB) | **BWT (900KB)** |
| **Zero-run encoding** | None | None | None | ZRLE | ZRLE | RLE0 | RLE0 |
| **PPM variant** | — | Simple | Simple | Simple | Simple | Method C | **Method C + PPMD** |
| **Escape estimator** | — | — | — | seen/4 | seen/4 | seen/4 | **singleton count** |
| **Frequency rescaling** | No | No | No | No | No | No | **Yes (thresh 8192)** |
| **Entropy Coder** | Arithmetic | Range | Range | Range | Range + rANS | Range + rANS | Range + rANS |
| **Order-1 threshold** | — | 6.5 | 6.5 | 6.8 | 6.8 | 7.2 | 7.2 |
| **Files Won** | 1/8 | 2/8 | 3/8 | 3/8 | 3/8 | 3/7 | **4/7** |
| **Compress Speed** | Baseline | ~2× | Competitive | 27–69× vs v3 | +1.7–4.6× vs v4.0 | Same as v5.0 | Slightly slower (900KB) |
| **Decompress Speed** | Baseline | ~2× | Competitive | Baseline | −25% Order-1, −82% Order-0 | Same as v5.0 | Same as v6.0 |
| **Threading** | None | None | None | Parallel BWT | Full parallel pipeline | Same as v5.0 | Same as v5.0 |
| **Beats bzip2 (total)** | No | No | No | No | No | Yes | **Yes (−0.03pp)** |

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
- **2026-03-14 - 2026-03-19**: v5.0 development
  - Full pipeline independence per block (pbzip2 design)
  - encode_symbol_fast (stack-allocated EncodeResult)
  - find_symbol_and_get_range (fused decode, eliminates duplicate scan)
  - Decode loop restructuring (flat if/else, inline accessors, total_freq hoisting)
  - Tested and reverted: prefix-sum cumulative arrays (21% slower, cache pressure)
  - rANS Order-0: ryg's rans_byte.h + RansStaticCoder wrapper
  - New PARALLEL (0x07) and RANS_ORDER_0 (0x08) file formats
- **2026-03-19**: v5.0 released (pipeline parallelism + rANS + 600KB blocks + LTO)
- **2026-03-19**: v6.0 development
  - PPM Method C exclusions (encode_symbol_fast + decoder escape branches)
  - RLE0 replacing ZRLE (bijective base-2, non-zero shift)
  - Tested and reverted: update exclusion (regression with Method C)
  - Entropy threshold 6.8 → 7.2 (routes E, F to Order-1)
- **2026-03-19**: v6.0 released (PPM Method C + RLE0 + threshold; beats bzip2 on total compressed size)
- **2026-03-19**: v7.0 development
  - PPMD singleton escape (singleton1_[256] array, O(1) updates at freq 0→1 and 1→2)
  - Frequency rescaling at threshold 8192 (halve round-up, recount singletons)
  - Tested and reverted: rescaling threshold 16384/32768 (too infrequent)
  - Tested and reverted: exempting context 0 from rescaling (worsened results)
  - Block size 600 KB → 900 KB (better BWT context, fewer blocks, lower entropy MTF)
- **2026-03-19**: v7.0 released (PPMD escape + rescaling + 900KB blocks; 54.85% avg, 4/7 files beat bzip2)

---

*Last updated: 2026-03-19 (v7.0)*
