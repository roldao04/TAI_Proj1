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
- `bin/g07-v5-c` - Compression tool (v5.0)
- `bin/g07-v5-d` - Decompression tool (v5.0)
- `bin/g07-v6-c` - Compression tool (v6.0) if v6 exists
- `bin/g07-v6-d` - Decompression tool (v6.0) if v6 exists

To clean build artifacts:

```bash
make clean
```

---

## Usage

### Compression

```bash
./bin/g07-v5-c <input_file> <output_file>
```

**Example:**
```bash
./bin/g07-v5-c data/A data/A.compressed
```

**Note:** v5.0 uses hardcoded optimal settings. No flags needed.
- Auto-selects model (Order-0 vs Order-1 based on entropy)
- Auto-enables BWT when entropy < 6.5 and file size > 10KB
- Auto-enables MTF + ZRLE when beneficial

### Decompression

```bash
./bin/g07-v5-d <compressed_file> <output_file>
```

**Example:**
```bash
./bin/g07-v5-d data/A.compressed data/A.decompressed
```

**Note:** Decompression automatically detects which model and transforms were used during compression.

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
- **Average Compression Ratio**: **54.73%** (v5.0)
  - Beats bzip2 by 0.20pp
  - Competitive ranking with industry tools
- **Speed**: ~25 MB/s compression speed
- **Binary Size**: < 120KB per binary (well under 1MB requirement)

---

## Version History

See [versions/VERSIONS.md](versions/VERSIONS.md) for detailed version history.

**Current Version**: 5.0 (April 2026)
- Simplified CLI (no flags needed - hardcoded optimal settings)
- Auto-selection for model, BWT, MTF, ZRLE
- BWT preprocessing (block-based, 900KB blocks)
- Multi-model system (Order-0 + Order-1)
- Range coding (faster than arithmetic)
- **54.73% average compression ratio**
- **~25 MB/s compression speed**

**Previous Versions**:
- **v4.0**: Advanced model improvements
- **v3.0**: BWT preprocessing introduced - 57.48% ratio
- **v2.0**: Multi-model + Range coding - 62.74% ratio
- **v1.0**: Order-0 + Arithmetic coding - 71.44% ratio

---

## Future Improvements

Potential enhancements (see [roadmap.md](roadmap.md) for detailed plans):
1. **Variable BWT Block Size**: Adaptive block size selection (1KB - 4KB)
2. **Parallel BWT Processing**: Multi-threaded block processing
3. **Order-1 Model Optimization**: Replace `std::map` contexts with denser structures
4. **Better Escape Handling**: Improve Order-1 fallback and exclusion heuristics
5. **Streaming Mode**: Support for files larger than RAM

**Note**: Order-2 model was tested and removed after benchmarks showed no benefit over Order-1.

---

## Attribution

- Range coding implementation: Based on Michael Schindler's range coder (GPL v2+)
- BWT implementation: Original work by project team (based on Burrows-Wheeler 1994 paper)
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
