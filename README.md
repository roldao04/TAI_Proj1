# Lossless Data Compression Tool

**TAI Project #1 - 2025/26**

**Algorithmic Information Theory**

**Universidade de Aveiro**

**Group 07**

---

## Overview

This project implements a lossless data compression tool using:
- **Burrows-Wheeler Transform (BWT)** for preprocessing (block-based, improves compression on repetitive data)
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
│   ├── transform/         # BWT (Burrows-Wheeler Transform)
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
- `bin/compress` - Compression tool
- `bin/decompress` - Decompression tool

To clean build artifacts:

```bash
make clean
```

---

## Usage

### Compression

```bash
./bin/compress <input_file> <compressed_file> [options]
```

**Options:**
- `--model auto|order0|order1` - Model selection (default: auto)
  - `auto` - Automatic selection based on file characteristics (recommended)
  - `order0` - Force Order-0 frequency model (fast, universal)
  - `order1` - Force Order-1 context model (better compression for low-entropy data)
- `--bwt` - Force BWT preprocessing (even if auto-selection says no)
- `--no-bwt` - Disable BWT preprocessing (revert to v2.0 behavior)
- `--yes, -y` - Skip interactive prompts (useful for automation/benchmarks)

**Examples:**
```bash
# Auto-selection (recommended - auto-selects model and BWT)
./bin/compress data/test.txt data/test.compressed

# Force BWT + Order-1 (maximum compression on text)
./bin/compress data/test.txt data/test.compressed --model order1 --bwt

# Disable BWT for speed
./bin/compress data/test.txt data/test.compressed --no-bwt

# Automated/batch processing (no prompts)
./bin/compress data/test.txt data/test.compressed --yes
```

**Auto-Selection Rules:**
- **BWT Selection:**
  - Enabled when entropy < 6.5 and file size >= 1024 bytes
  - Disabled for high-entropy or very small files
- **Model Selection:**
  - Entropy > 7.5 → Store uncompressed (incompressible data)
  - Entropy 6.8-7.5 → Order-0 (high entropy, Order-1 overhead not worth it)
  - File < 100KB → Order-0 (adaptive overhead not worthwhile)
  - Otherwise → Order-1 (low entropy, achieves 20-50% better compression)

### Decompression

```bash
./bin/decompress <compressed_file> <output_file>
```

Example:
```bash
./bin/decompress data/test.compressed data/test.decompressed
```

Note: Decompression automatically detects which model was used during compression.

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
| Model Type | Original Size | BWT Flag | BWT Indices | Model Data | Encoded Data |
| (1 byte)   | (8 bytes)     | (1 byte) | (varies)    | (varies)   | (variable)   |
```

- **Model Type**: 0 = Order-0, 1 = Order-1, 255 = Uncompressed
- **Original Size**: uint64_t, little-endian
- **BWT Flag**: 0x00 = no BWT, 0x01 = BWT enabled
- **BWT Indices**: If BWT enabled, primary indices for each 1024-byte block (4 bytes each)
- **Model Data**:
  - Order-0: 257 × uint32_t frequency table (1028 bytes)
  - Order-1: No data stored (adaptive model builds during decode)
  - Uncompressed: No data
- **Encoded Data**: Range-coded bitstream (or raw data if uncompressed)

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

- Block-based preprocessing (1024-byte blocks)
- Reversible transformation that groups similar characters
- Suffix array construction for forward transform
- LF mapping for efficient inverse transform
- Auto-enabled for entropy < 6.5 and file size >= 1KB
- Improves compression by 8-20% on text and structured data
- Seamlessly integrated with range coding and models

---

## Performance Notes

- **Memory**: Uses ~8GB max (requirement compliant)
- **Optimization**: Compiled with `-O3` for performance
- **Platform**: Tested on Linux
- **Average Compression Ratio**: **57.48%** (v3)
  - v2.0: 62.74% | v1.0: 71.44%
  - **19.5% improvement over v1.0**
  - **8.4% improvement over v2.0**
- **Overall Ranking**: **#3** out of 5 tools (xz, bzip2, g07, zstd, gzip)
- **Files Won**: 3/8 files (Files A, D, G)
  - File A (text): 51.96% - Best among all tools
  - File G (structured): 27.92% - Best among all tools
- **Speed**: Competitive with multi-model system, BWT adds minimal overhead

---

## Version History

See [versions/VERSIONS.md](versions/VERSIONS.md) for detailed version history and [versions/g07_v3.0/README.md](versions/g07_v3.0/README.md) for comprehensive documentation.

**Current Version**: 3.0 (March 2026)
- **BWT preprocessing** (block-based, 1024-byte blocks)
- Multi-model system (Order-0 + Order-1)
- Range coding (faster than arithmetic)
- Intelligent auto-selection (BWT + model)
- **57.48% average compression ratio**
- **#3 ranking** vs industry tools

**Previous Versions**:
- **v2.0** (March 2026): Multi-model + Range coding - 62.74% ratio
- **v1.0** (February 2026): Order-0 + Arithmetic coding - 71.44% ratio

---

## Future Improvements

Potential enhancements (see [versions/g07_v3.0/README.md](versions/g07_v3.0/README.md#future-improvements) for details):
1. **Variable BWT Block Size**: Adaptive block size selection (1KB - 4KB)
2. **Move-to-Front Transform**: Additional transform after BWT (used in bzip2)
3. **Parallel BWT Processing**: Multi-threaded block processing
4. **Run-Length Encoding**: Pre-process runs after BWT
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
