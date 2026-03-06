# Version History - TAI Project #1

**Lossless Data Compression Tool**

Group 07 - Universidade de Aveiro

---

## Version Overview

| Version | Date | Key Features | Performance | Status |
|---------|------|--------------|-------------|--------|
| **v2.0** | 2026-03-05 | Multi-Model + Range Coding + Auto-Select | 62.74% avg ratio | **Current** |
| **v1.0** | 2026-02-20 | Order-0 Model + Arithmetic Coding | 71.44% avg ratio | Superseded |

---

## Version Details

### v2.0 (March 2026) - Current Release

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

| Metric | v1.0 | v2.0 | Improvement |
|--------|------|------|-------------|
| **Compression Ratio** | 71.44% | 62.74% | **-8.7pp (13% better)** |
| **Bits/Symbol** | 5.72 | 5.02 | -0.70 |
| **Files Won** | 1/8 | 2/8 | +1 file |
| **Speed** | Baseline | ~2x faster | **100% speedup** |
| **Models** | 1 (Order-0) | 2 (Order-0 + Order-1) | +1 model |
| **Auto-Selection** | No | Yes (90% success) | ✅ Added |
| **Incompressible Detection** | No | Yes | ✅ Added |
| **CLI Options** | None | --model | ✅ Enhanced |

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

---

*Last updated: 2026-03-05*
