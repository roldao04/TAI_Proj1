# Version History - TAI Project #1

**Lossless Data Compression Tool**

Group 07 - Universidade de Aveiro

---

## Version Overview

| Version | Date | Key Features | Performance | Status |
|---------|------|--------------|-------------|--------|
| **v5.0** | 2026-03-19 | Full parallelism + rANS + PPM Method C + PPMD escape + dynamic blocks | 54.73% avg ratio | **Current** |
| **v4.0** | 2026-03-13 | Flat array context model + libsais + AVX2 + parallel BWT | 56.39% avg ratio | Superseded |
| **v3** | 2026-03-06 | BWT Preprocessing + Multi-Model + Range Coding | 57.48% avg ratio | Superseded |
| **v2.0** | 2026-03-05 | Multi-Model + Range Coding + Auto-Select | 62.74% avg ratio | Superseded |
| **v1.0** | 2026-02-20 | Order-0 Model + Arithmetic Coding | 71.44% avg ratio | Superseded |

---

## Version Details

### v5.0 (March 2026) - Current Release

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

| Metric | v1.0 | v2.0 | v3 | v4.0 | v5.0 |
|--------|------|------|------|------|------|
| **Compression Ratio** | 71.44% | 62.74% | 57.48% | 56.39% | **54.73%** |
| **Bits/Symbol** | 5.72 | 5.02 | 4.60 | 4.51 | **4.38** |
| **Preprocessing** | None | None | BWT (1024B) | BWT (900KB) | **BWT (dynamic ≤2000KB)** |
| **Zero-run encoding** | None | None | None | ZRLE | **RLE0 (rank-255 safe)** |
| **PPM variant** | — | Simple | Simple | Simple | **Method C + PPMD** |
| **Escape estimator** | — | — | — | seen/4 | **singleton count** |
| **Frequency rescaling** | No | No | No | No | **Yes (thresh 8192)** |
| **Entropy Coder** | Arithmetic | Range | Range | Range | **Range + rANS** |
| **Order-1 threshold** | — | 6.5 | 6.5 | 6.8 | **7.2** |
| **Threading** | None | None | None | Parallel BWT | **Full parallel pipeline** |
| **Decode Optimization** | Baseline | ~2× | Competitive | Baseline | **−38% Order-1, −82% Order-0** |
| **Files Won vs bzip2** | 1/8 | 2/8 | 3/8 | 3/8 | **5/8** |
| **Compress Speed vs v4.0** | — | — | — | Baseline | **+1.7–5.4×** |
| **Decompress Speed vs v4.0** | — | — | — | Baseline | **−25% to −82%** |
| **Beats bzip2 (total)** | No | No | No | No | **Yes (−0.20pp)** |

---

## Performance Timeline

### Compression Ratio Progress

| Version | Avg Ratio | Improvement vs Previous | Cumulative Improvement |
|---------|-----------|------------------------|------------------------|
| v1.0 | 71.44% | — | — |
| v2.0 | 62.74% | −8.70pp | −8.70pp |
| v3 | 57.48% | −5.26pp | −13.96pp |
| v4.0 | 56.39% | −1.09pp | −15.05pp |
| v5.0 | **54.73%** | **−1.66pp** | **−16.71pp** |

### Speed Evolution

| Version | Compression | Decompression | Key Technique |
|---------|-------------|---------------|---------------|
| v1.0 | ~98ms avg | ~127ms avg | Baseline arithmetic |
| v2.0 | ~2× faster | ~2× faster | Range coding |
| v3 | Competitive | Competitive | BWT + range |
| v4.0 | 27–69× vs v3 | Baseline | Flat arrays + libsais + AVX2 |
| v5.0 | **+1.7–5.4× vs v4** | **−25% to −82% vs v4** | Full parallelism + rANS + LTO |

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

---

## Notable Achievements

### Version 5.0 Highlights
- **Best compression ratio achieved**: 54.73% average (beats bzip2 by 0.20pp)
- **Fastest compression**: 2-5× speedup vs v4.0 through full parallelism
- **Fastest decompression**: −82% on high-entropy files (rANS), −38% on text/structured (decode optimizations)
- **Most files won**: 5/8 files beat bzip2 (A, B, C, D, E)
- **Largest single-file improvement**: File F −5.92pp (87.62% → 81.70%) via threshold adjustment
- **Most sophisticated model**: Method C + PPMD singleton escape + rescaling

### Overall Project Achievements
- **16.71pp cumulative improvement** from v1.0 to v5.0 (71.44% → 54.73%)
- **Competitive with industry leaders**: Beats bzip2, competitive with xz (LZMA)
- **Comprehensive approach**: Combines BWT preprocessing, adaptive PPM modeling, multiple entropy coders
- **Production-ready**: Parallel processing, robust error handling, extensive testing
- **Well-documented**: Detailed READMEs track all experiments including failed optimizations

---

## Files by Version

| Version | Binaries | Documentation |
|---------|----------|---------------|
| v1.0 | — | README.md |
| v2.0 | — | README.md |
| v3.0 | — | README.md |
| v4.0 | G07-V4-C, G07-V4-D | README.md |
| v5.0 | — | README.md (comprehensive) |

**Note:** v4.0 is the last version with preserved binaries. v5.0 source code represents the current implementation in the main branch.

---

*Last updated: 2026-03-19 (v5.0)*
