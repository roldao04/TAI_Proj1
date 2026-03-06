# Lossless Data Compression Tool

**TAI Project #1 - 2025/26**

**Algorithmic Information Theory**

**Universidade de Aveiro**

**Group 07**

---

## Overview

This project implements a lossless data compression tool using:
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
│   ├── arithmetic/         # Arithmetic encoder/decoder
│   ├── model/             # Frequency-based statistical model
│   ├── utils/             # File I/O utilities
│   ├── compressor.cpp     # Main compression program
│   └── decompressor.cpp   # Main decompression program
├── include/               # Header files
├── tests/                 # Verification tests
├── benchmarks/            # Benchmarking scripts
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
./bin/compress <input_file> <compressed_file> [--model order0|order1|auto] [--yes]
```

**Model Options:**
- `--model auto` - Automatic selection based on file characteristics (default, recommended)
- `--model order0` - Force Order-0 frequency model (fast, universal)
- `--model order1` - Force Order-1 context model (better compression for low-entropy data)
- `--yes, -y` - Skip interactive prompts (useful for automation/benchmarks)

**Examples:**
```bash
# Auto-selection (recommended)
./bin/compress data/test.txt data/test.compressed

# Force Order-0 (fastest)
./bin/compress data/test.txt data/test.compressed --model order0

# Force Order-1 (maximum compression)
./bin/compress data/test.txt data/test.compressed --model order1

# Automated/batch processing (no prompts)
./bin/compress data/test.txt data/test.compressed --model order1 --yes
```

**Auto-Selection Rules:**
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

Run the lossless verification test suite:

```bash
make test
```

This tests:
1. Text files
2. Binary data
3. Empty files
4. Highly repetitive data

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

### Compare Models

Compare Order-0 vs Order-1 vs Auto-selection:

```bash
./benchmarks/benchmark_models.sh
```

**Output files:**
- `benchmarks/model_comparison.csv` - Model performance data
- `benchmarks/model_comparison.md` - Detailed analysis of each model

### Compare Versions

Compare v1.0 vs current implementation (v2.0):

```bash
./benchmarks/benchmark_versions.sh
```

**Output files:**
- `benchmarks/version_comparison.csv` - Version performance data
- `benchmarks/version_comparison.md` - Detailed version comparison analysis

---

## Implementation Details

### Compressed File Format

```
| Model Type (1 byte) | Original Size (8 bytes) | Model Data (varies) | Encoded Data |
```

- **Model Type**: 0 = Order-0, 1 = Order-1, 255 = Uncompressed
- **Original Size**: uint64_t, little-endian
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

---

## Performance Notes

- **Memory**: Uses ~8GB max (requirement compliant)
- **Optimization**: Compiled with `-O3` for performance
- **Platform**: Tested on Linux
- **Average Compression Ratio**: ~62% (v2.0) vs ~71% (v1.0) - 13% improvement
- **Speed**: Range coding provides ~2x speedup over arithmetic coding
- **Files Won**: Beats gzip on text files, competitive with bzip2 on structured data

---

## Version History

See [versions/VERSIONS.md](versions/VERSIONS.md) for detailed version history.

**Current Version**: 2.0 (March 2026)
- Multi-model system (Order-0 + Order-1)
- Range coding (faster than arithmetic)
- Intelligent auto-selection
- ~13% better compression than v1.0

**Previous Version**: 1.0 (February 2026)
- Order-0 + Arithmetic coding
- 71.44% average compression ratio

---

## Next Steps / Improvements

Potential enhancements:
1. **BWT Transform**: Preprocessing like bzip2 for 20-40% better text compression
2. **Order-2 Model**: Experimental (removed after benchmarks showed no benefit)
3. **Parallel Processing**: Multi-threaded compression for large files
4. **Streaming Mode**: Support for files larger than RAM

---

## Attribution

- Range coding implementation: Based on Michael Schindler's range coder (GPL v2+)
- Order-0 model implementation: Original work by project team
- Order-1 adaptive PPM model: Original work by project team
- Auto-selection algorithm: Original work by project team

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## License

Educational project for TAI course at Universidade de Aveiro.
