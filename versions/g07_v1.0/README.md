# Version 1.0 - Detailed Documentation

**Release Date:** February 20, 2026

**Status:** Released

---

## Overview

This is the initial release of the TAI Group 07 lossless data compression tool. It implements a foundational compression system using Order-0 frequency modeling and arithmetic coding.

---

## Core Components

### 1. Order-0 Frequency Model

**Implementation:**
- Single-pass frequency analysis of input data
- Counts occurrence of each byte (0-255)
- Builds cumulative frequency table for arithmetic coding
- Includes special EOF marker (symbol 256)

**Characteristics:**
- No context awareness (each byte treated independently)
- Minimum frequency of 1 assigned to all symbols (avoids zero probabilities)
- Simple and fast - suitable for baseline implementation

**Limitations:**
- Does not exploit sequential patterns in data
- Less effective on structured or repetitive data
- Cannot adapt to changing data characteristics

### 2. Arithmetic Coding

**Implementation Details:**
- 32-bit precision arithmetic
- Renormalization to prevent underflow
- Bit-by-bit output with carry propagation handling
- Based on algorithms from Khalid Sayood's "Introduction to Data Compression"

**Features:**
- Near-optimal entropy encoding
- Handles fractional bits per symbol
- Deterministic encoding/decoding

### 3. File Format

**Compressed File Structure:**
```
+-------------------+------------------------+----------------+
| Original Size     | Frequency Table        | Encoded Data   |
| (8 bytes)         | (1028 bytes)           | (variable)     |
+-------------------+------------------------+----------------+
```

**Header Details:**
- **Original Size**: uint64_t (little-endian) - size of uncompressed data
- **Frequency Table**: 257 entries × 4 bytes each
  - Entries 0-255: byte frequencies
  - Entry 256: EOF marker
- **Encoded Data**: Arithmetic-coded bitstream

---

## Performance Analysis

### Benchmark Results (8 test files, 13MiB total)

**Compression Metrics:**
- Total original size: 13 MiB
- Total compressed: 9.0 MiB
- Average compression ratio: **71.44%**
- Average bits/symbol: **5.72**

**Speed Metrics:**
- Average compression time: 98 ms
- Average decompression time: 127 ms

**Comparison with Industry Tools:**
| Rank | Tool | Avg Ratio | Speed |
|------|------|-----------|-------|
| 1 | xz | 54.16% | Slow (378ms compress) |
| 2 | bzip2 | 54.88% | Medium (118ms compress) |
| 3 | zstd | 58.59% | Very Fast (15ms compress) |
| 4 | gzip | 59.47% | Fast (75ms compress) |
| 5 | **our (v1.0)** | **71.44%** | Medium (98ms compress) |

**Best Performance On:**
- File A (text, 1.3MiB): 52.05% compression ratio (best among all tools tested)

**Worst Performance On:**
- File D (random data, 2.0MiB): 100.05% (slight expansion - expected for random data)
- File E (binary, 989KiB): 88.22%

---

## Build Information

**Compilation:**
- Compiler: g++ with C++11 standard
- Optimization: -O3
- Platform: Linux (tested on kernel 6.17.0-14-generic)

**Binaries Included:**
- `compress` (43824 bytes) - Compression tool
- `decompress` (39728 bytes) - Decompression tool

**Memory Usage:**
- Maximum: ~8GB (compliant with requirements)
- Typical: Much lower for most files

---

## Usage

### Compression
```bash
./compress <input_file> <compressed_file>
```

**Example:**
```bash
./compress input.txt output.compressed
```

### Decompression
```bash
./decompress <compressed_file> <output_file>
```

**Example:**
```bash
./decompress output.compressed recovered.txt
```

### Verification
Both tools output timing information and file sizes to stderr.

---

## Known Limitations

1. **No Context Modeling**: Order-0 model doesn't use any context, limiting compression on structured data

2. **Static Frequency Table**: Frequencies counted in first pass, not adapted during compression

3. **Small File Overhead**: 1036-byte header is significant for files under ~10KB

4. **Random Data**: Cannot compress truly random data (expected behavior)

5. **Speed**: Slower than modern tools like zstd due to:
   - Two-pass approach (frequency counting + encoding)
   - Non-optimized arithmetic operations

---

## Testing

**Verification Status:** ✓ All tests passed
- Text files: Perfect reconstruction
- Binary data: Perfect reconstruction
- Empty files: Handled correctly
- Highly repetitive data: Successful compression

**Lossless Guarantee:** 100% byte-for-byte accuracy verified on all test cases

---

## Technical Notes

### Why Order-0 is Limited

Order-0 treats each byte independently:
```
Input: "aaaaabbbbb"
Model: P(a)=0.5, P(b)=0.5
Encoding: Each symbol uses -log2(0.5) = 1 bit
Total: 10 bits
```

Order-1 would recognize patterns:
```
Input: "aaaaabbbbb"
Model: P(a|a)≈1, P(b|b)≈1, P(b|a)≈1 (at transition)
Encoding: Most symbols use ≈0 bits (high probability)
Total: Much fewer bits
```

### When v1.0 Works Well

Best for:
- Text with balanced character distribution
- First-time compression tasks
- When speed matters more than ratio
- Educational purposes (clear, simple implementation)

Not ideal for:
- Highly structured data (XML, JSON)
- Repetitive sequences
- Binary executables
- Already compressed data

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## References

- Khalid Sayood, "Introduction to Data Compression" (4th Edition)
- TAI Course Materials, Universidade de Aveiro

---

*Documentation last updated: 2026-02-26*
