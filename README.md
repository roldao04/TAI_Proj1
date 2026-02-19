# Lossless Data Compression Tool

**TAI Project #1 - 2025/26**
**Algorithmic Information Theory**
**Universidade de Aveiro**

---

## Overview

This project implements a lossless data compression tool using:
- **Arithmetic Coding** for entropy coding
- **Order-0 Frequency Model** for probability estimation

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
./bin/compress <input_file> <compressed_file>
```

Example:
```bash
./bin/compress data/test.txt data/test.compressed
```

### Decompression

```bash
./bin/decompress <compressed_file> <output_file>
```

Example:
```bash
./bin/decompress data/test.compressed data/test.decompressed
```

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

---

## Implementation Details

### Compressed File Format

```
| Original Size (8 bytes) | Frequency Table (1028 bytes) | Encoded Data |
```

- **Original Size**: uint64_t, little-endian
- **Frequency Table**: 257 × uint32_t (256 bytes + 1 EOF symbol)
- **Encoded Data**: Arithmetic coded bitstream

### Arithmetic Coding

Implementation based on standard algorithms from:
> Khalid Sayood, "Introduction to Data Compression"

Features:
- 32-bit precision
- Renormalization to prevent underflow
- Bit-by-bit output with carry handling

### Order-0 Model

- Counts byte frequency in input
- Builds cumulative frequency table
- All symbols assigned minimum frequency of 1 (avoid zero probabilities)
- Includes EOF marker for proper termination

---

## Performance Notes

- **Memory**: Uses ~8GB max (requirement compliant)
- **Optimization**: Compiled with `-O3` for performance
- **Platform**: Tested on Linux

---

## Next Steps / Improvements

To enhance compression:
1. **Order-k Markov Model**: Use context for better prediction
2. **Adaptive Model**: Update probabilities during encoding
3. **BWT Transform**: Preprocessing like bzip2
4. **File-type Detection**: Different models for different data types

---

## Attribution

- Arithmetic coding implementation: Based on standard algorithms
- Model implementation: Original work by project team

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## License

Educational project for TAI course at Universidade de Aveiro.
