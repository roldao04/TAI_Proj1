# Lossless Data Compression Tool

**TAI Project #1 - 2025/26**

**Algorithmic Information Theory**

**Universidade de Aveiro**

**Group 07**

---

## Overview

This project implements a lossless data compression tool using:
- **Burrows-Wheeler Transform (BWT)** for preprocessing (block-based, improves compression on repetitive data)
- **Move-to-Front (MTF)** after BWT to convert clustered symbols into low-rank bytes
- **Optional zero-run RLE** after MTF to compact long runs of zeros
- **Range Coding** for entropy coding (faster than arithmetic coding)
- **Multi-Model System** with intelligent auto-selection:
  - **Order-0** frequency model (static, fast, universal)
  - **Order-1** adaptive context model (PPM-style, better compression for low-entropy data)
- **Auto-Selection Algorithm** based on file entropy and size

The system guarantees perfect reconstruction of the original data.

---

## Project Structure

```
project_1/
├── src/
│   ├── arithmetic/         # Range coder (entropy encoding)
│   ├── model/             # Frequency models (Order-0, Order-1)
│   ├── transform/         # BWT, MTF, and zero-run preprocessing
│   ├── utils/             # File I/O and entropy calculation
│   ├── compressor.cpp     # Main compression program
│   └── decompressor.cpp   # Main decompression program
├── include/               # Header files
├── tests/                 # Verification and unit tests
├── benchmarks/            # Benchmarking scripts
├── versions/              # Version history and binaries
├── data/                  # Test data files
├── Makefile              # Build system
└── README.md             # This file
```

---

## Building

Compile both compressor and decompressor:

```bash
make
```

This creates:
- `bin/g07-v5-c` - Compression tool (v5.0 - best compression ratio)
- `bin/g07-v5-d` - Decompression tool (v5.0)
- `bin/g07-v6-c` - Compression tool (v6.0) if v6 exists
- `bin/g07-v6-d` - Decompression tool (v6.0) if v6 exists
- `bin/g07-v7-c` - Compression tool (v7.0 - speed-optimized)
- `bin/g07-v7-d` - Decompression tool (v7.0)

To clean build artifacts:

```bash
make clean
```

---

## Usage

### Compression

Two production versions are available — choose based on your priority:

**v5.0 — Best Compression Ratio (54.70% avg)**
```bash
./bin/g07-v5-c <input_file> <output_file>
```
- BWT + MTF + ZRLE + Order-1 adaptive PPM + Range Coder
- Beats bzip2 on 5/8 files; competitive with xz

**v7.0 — Maximum Speed (28.9 MB/s compress, 51.1 MB/s decompress)**
```bash
./bin/g07-v7-c <input_file> <output_file>
```
- BWT + MTF + 2-Way Interleaved rANS Order-0
- 1.7x faster compression, 3x faster decompression vs v5
- 59.2% avg ratio (still well under 70% target)
- On high-entropy files: up to 13x faster than v5

**Example:**
```bash
./bin/g07-v5-c data/A data/A.compressed   # best ratio
./bin/g07-v7-c data/A data/A.fast         # best speed
```

### Decompression

```bash
./bin/g07-v5-d <compressed_file> <output_file>   # for v5 files
./bin/g07-v7-d <compressed_file> <output_file>   # for v7 files
```

Each decompressor auto-detects the format used during compression. v5 files cannot be decompressed by v7-d and vice versa (each prints a clear error if the wrong tool is used).

**Example:**
```bash
./bin/g07-v5-d data/A.compressed data/A.decompressed
./bin/g07-v7-d data/A.fast       data/A.decompressed
```

---

## Verification

Run the comprehensive test suite:

```bash
make test
```

This runs:
1. **BWT Unit Tests** - Transform/inverse, block processing, edge cases
2. **Integration Tests** - Lossless compression/decompression on:
   - Text files
   - Binary data
   - Empty files
   - Highly repetitive data

All tests verify byte-for-byte identical reconstruction.

---

## Benchmarking

### Compare Against Standard Tools

Run comprehensive benchmarks on all data files:

```bash
make benchmark
```

This will:
1. Test all files in `data/` directory (A through H)
2. Compare against gzip, bzip2, xz, and zstd
3. Report compression ratio, bits/symbol, and timing for each tool
4. Generate results in CSV and Markdown formats

**Output files:**
- `benchmarks/results.csv` - Machine-readable data for analysis
- `benchmarks/results.md` - Markdown table for reports

**Single file benchmark:**
```bash
./benchmarks/compare.sh <file>
```

This compares G07 against gzip, bzip2, xz, and zstd on a single file.

---

## Implementation Details

### Compressed File Format

```
| Model Type | Original Size | Optional Transform Header | BWT Indices | Model Data | Encoded Data |
| (1 byte)   | (8 bytes)     | (flags + preproc size)    | (varies)    | (varies)   | (variable)   |
```

- **Model Type**:
  - `0` = Order-0
  - `1` = Order-1
  - `3` = Order-0 + BWT (legacy format)
  - `4` = Order-1 + BWT (legacy format)
  - `5` = Order-0 + extended preprocessing header
  - `6` = Order-1 + extended preprocessing header
  - `255` = Uncompressed
- **Original Size**: uint64_t, little-endian
- **Optional Transform Header** (for model types `5` and `6`):
  - `transform_flags` (1 byte): bit0 = BWT, bit1 = MTF, bit2 = zero-run RLE
  - `preprocessed_size` (8 bytes): byte count after all preprocessing, before range coding
- **BWT Indices**: If BWT is enabled, store the block count followed by one primary index per 900KB block (4 bytes each)
- **Model Data**:
  - Order-0: 257 × uint32_t frequency table (1028 bytes)
  - Order-1: No data stored (adaptive model builds during decode)
  - Uncompressed: No data
- **Encoded Data**: Range-coded bitstream (or raw data if uncompressed)

**Current preprocessing pipeline:**
- Legacy BWT streams: `input -> BWT -> model -> range coder`
- Extended streams: `input -> BWT -> MTF -> optional zero-run RLE -> model -> range coder`

### Range Coding

Implementation based on Michael Schindler's range coder:
> Schindler, M. (1998). "A Fast Renormalisation for Arithmetic Coding"

Features:
- Faster than arithmetic coding (~2x speedup)
- Same compression efficiency
- Renormalization in larger units than bits
- Minimal file size overhead (< 0.01%)

### Order-0 Model (Static)

- Counts byte frequency in input (first pass)
- Builds cumulative frequency table
- All symbols assigned minimum frequency of 1 (avoid zero probabilities)
- Includes EOF marker for proper termination
- Fast and universal

### Order-1 Model (Adaptive PPM)

- Uses previous byte as context (256 possible contexts)
- Adaptive frequency updates during encoding/decoding
- Escape mechanism for unseen symbols
- Simplified encoding (no PPM Method C exclusions)
- No model data stored (builds during decode)
- 20-50% better compression on low-entropy data

### Burrows-Wheeler Transform (BWT)

- Block-based preprocessing (900KB blocks)
- Reversible transformation that groups similar characters
- Suffix array construction for forward transform
- LF mapping for efficient inverse transform
- Auto-enabled for entropy < 6.5 and file size > 10KB
- Improves compression by 8-20% on text and structured data
- Seamlessly integrated with range coding and models

### Move-to-Front (MTF)

- Applied after BWT unless `--no-mtf` is used
- Converts each byte into its current rank in a dynamic 256-symbol list
- Produces many low-valued bytes after BWT clusters similar symbols together
- Resets its state at each BWT block boundary

### Zero-Run RLE

- Applied after MTF when requested, or automatically when it shrinks the MTF stream
- Compresses long runs of MTF zeros using an escaped marker scheme
- Reversed before inverse MTF during decompression

---

## Performance Notes

- **Memory**: Uses ~1-2GB (requirement compliant)
- **Optimization**: Compiled with `-O3 -march=native -flto=auto` for maximum performance
- **Platform**: Tested on Linux
- **Binary Size**: < 120KB per binary (well under 1MB requirement)

### Two Production Versions

| Metric | v5 (Best Compression) | v7 (Best Speed) |
|--------|----------------------|-----------------|
| **Avg Compression Ratio** | **54.73%** | 59.2% |
| **Compress Speed** | ~25 MB/s | **28.9 MB/s** |
| **Decompress Speed** | ~17 MB/s | **51.1 MB/s** |
| **Pipeline** | BWT+MTF+ZRLE+Order-1+Range | BWT+MTF+Interleaved rANS |
| **Best For** | Archival, max compression | Real-time, throughput |

---

## Version History

See [versions/VERSIONS.md](versions/VERSIONS.md) for detailed version history.

**v7.0 (April 2026) - Speed-Optimized**
- BWT + MTF + 2-way interleaved rANS Order-0
- **59.2% avg compression ratio** at **51.1 MB/s decompression** (3x faster than v5)
- Adaptive: BWT+MTF for low-entropy, raw rANS for high-entropy, uncompressed for random
- Smallest binaries: 84KB compressor, 56KB decompressor

**v5.0 (March 2026) - Compression-Optimized**
- BWT + MTF + ZRLE + Order-1 adaptive PPM + Range Coding
- **54.73% avg compression ratio** (beats bzip2 by 0.20pp)
- Full pipeline parallelism, ~25 MB/s compression speed

**Previous Versions**:
- **v4.0**: Flat array context model + libsais + parallel BWT - 56.39% ratio
- **v3.0**: BWT preprocessing introduced - 57.48% ratio
- **v2.0**: Multi-model + Range coding - 62.74% ratio
- **v1.0**: Order-0 + Arithmetic coding - 71.44% ratio

---

## Attribution

- Range coding implementation: Based on Michael Schindler's range coder (GPL v2+)
- rANS implementation: Based on Fabian Giesen's ryg_rans (Public Domain)
- Interleaved rANS: Inspired by Giesen, "Interleaved entropy coders" (arXiv:1402.3392)
- ANS theory: Jarek Duda, "Asymmetric numeral systems" (arXiv:1311.2540)
- BWT implementation: Original work by project team (based on Burrows-Wheeler 1994 paper)
- libsais: Ilya Grebnov, linear-time suffix array construction
- Order-0 model implementation: Original work by project team
- Order-1 adaptive PPM model: Original work by project team
- Auto-selection algorithms: Original work by project team

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## License

Educational project for TAI course at Universidade de Aveiro.
