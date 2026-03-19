# Version History - TAI Project #1

**Lossless Data Compression Tool**

Group 07 - Universidade de Aveiro

---

## Version Overview

| Version | Date | Key Features | Performance | Status |
|---------|------|--------------|-------------|--------|
| **v5.0** | 2026-03-19 | Full pipeline parallelism + allocation-free encode loop | 56.39% avg ratio | **Current** |
| **v4.0** | 2026-03-13 | Flat array context model + libsais + AVX2 + parallel BWT | 56.39% avg ratio | Superseded |
| **v3** | 2026-03-06 | BWT Preprocessing + Multi-Model + Range Coding | 57.48% avg ratio | Superseded |
| **v2.0** | 2026-03-05 | Multi-Model + Range Coding + Auto-Select | 62.74% avg ratio | Superseded |
| **v1.0** | 2026-02-20 | Order-0 Model + Arithmetic Coding | 71.44% avg ratio | Superseded |

---

## Version Details

### v5.0 (March 2026) - Current Release

**Major Updates:**
- **Full Pipeline Parallelism**: Each 900 KB block runs its complete BWT → MTF → ZRLE → Order-1 pipeline in its own `std::thread` simultaneously (pbzip2 design)
- **Allocation-Free Encode Loop**: New `encode_symbol_fast()` returns a fixed-size `EncodeResult` struct instead of `std::vector<EncodingStep>`, eliminating ~900,000 heap allocations per block
- **New PARALLEL File Format**: Model type `0x07` with per-block metadata (BWT index, transform flags, sizes) instead of a single shared header

**Performance:**
- Average compression ratio: **56.39%** (unchanged from v4.0 — pure speed release)
- Compression: G 117ms→62ms (1.89×), B 68ms→39ms (1.74×), C 96ms→45ms (2.13×)
- Decompression: **−25% average** across all 7 files; now beats bzip2 on 6/7 files
- Decompress: A 63ms→43ms, B 43ms→32ms, C 48ms→34ms, G 43ms→34ms, H 97ms→76ms

**Failed optimizations tried:**
- Prefix-sum arrays for O(1) cumulative lookups: increased L2/L3 cache working set from ~264 KB to ~518 KB; made G 21% *slower*; reverted

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

| Metric | v1.0 | v2.0 | v3 | Improvement (v3 vs v1.0) |
|--------|------|------|------|---------------------------|
| **Compression Ratio** | 71.44% | 62.74% | **57.48%** | **-13.96pp (19.5% better)** |
| **Bits/Symbol** | 5.72 | 5.02 | **4.60** | **-1.12 bits** |
| **Preprocessing** | None | None | BWT (1024-byte blocks) | ✅ Added |
| **Files Won** | 1/8 | 2/8 | **3/8** | **+2 files** |
| **Overall Rank** | #5 | #5 | **#3** | **+2 positions** |
| **Speed** | Baseline | ~2x faster | Competitive | ~1.7× faster (v5.0 vs v4.0) |
| **Models** | 1 (Order-0) | 2 (Order-0 + Order-1) | 2 + BWT | Enhanced |
| **Auto-Selection** | No | Yes (90% success) | Yes (BWT + Model) | ✅ Enhanced |
| **BWT Auto-Select** | N/A | N/A | Yes (entropy-based) | ✅ Added |
| **Incompressible Detection** | No | Yes | Yes | ✅ Maintained |
| **CLI Options** | None | --model | --model, --bwt, --no-bwt | ✅ Enhanced |

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
  - Tested and reverted: prefix-sum cumulative arrays
  - New PARALLEL (0x07) file format
- **2026-03-19**: v5.0 released (pipeline parallelism, ~1.7× vs v4.0)

---

*Last updated: 2026-03-19*
