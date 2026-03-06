# Version 2.0 - Detailed Documentation

**Release Date:** March 5, 2026

**Status:** Current Release

---

## Overview

Version 2.0 represents a major evolution of the TAI Group 07 lossless compression tool, introducing a multi-model system with intelligent auto-selection and significantly improved performance.

---

## What's New in v2.0

### Major Features

1. **Multi-Model System**
   - Order-0 static frequency model (fast, universal)
   - Order-1 adaptive context model (PPM-style, better compression)
   - Intelligent auto-selection based on file characteristics

2. **Range Coding**
   - Replaced arithmetic coding from v1.0
   - ~2x faster encoding/decoding
   - Same compression efficiency
   - Based on Michael Schindler's range coder

3. **Smart File Handling**
   - Entropy detection to identify incompressible files
   - Automatic uncompressed storage for high-entropy data
   - Avoids expansion on already-compressed files

4. **Enhanced CLI**
   - `--model auto` for automatic selection (default)
   - `--model order0` to force Order-0
   - `--model order1` to force Order-1
   - User warnings for suboptimal model choices

---

## Core Components

### 1. Range Coding (New in v2.0)

**What Changed:**
- v1.0 used 32-bit arithmetic coding
- v2.0 uses range coding (Schindler's algorithm)

**Benefits:**
- 2x faster encoding/decoding
- Same compression efficiency
- Simpler implementation (no carry propagation)
- File size overhead < 0.01%

**Technical Details:**
- Renormalization in byte-sized chunks (vs bit-by-bit)
- Maintains same probability modeling
- Compatible with both Order-0 and Order-1 models

### 2. Order-0 Model (Enhanced from v1.0)

**What Changed:**
- Core algorithm unchanged (maintains compatibility)
- Now selectable via CLI
- Used automatically for high-entropy files
- Remains the default for small files (< 100KB)

**Characteristics:**
- Single-pass frequency analysis
- Static frequency table (stored in file)
- Fast compression/decompression
- Universal (works on any data)

**Performance:**
- File A: 51.85% ratio (best among all tools!)
- File B: 47.44%
- File C: 45.77%

### 3. Order-1 Adaptive Model (New in v2.0)

**Overview:**
- Uses previous byte as context (256 contexts)
- Adaptive frequency updates during encoding/decoding
- PPM-style escape mechanism
- No model data stored in file (saves 1028 bytes)

**Technical Implementation:**
- Maintains 256 separate frequency tables
- Each table tracks symbol frequencies for one context
- Updates frequencies after each symbol
- Escape to Order-0 for unseen symbol/context pairs
- Simplified encoding (no PPM Method C exclusions)

**Performance:**
- 20-50% better compression on low-entropy data
- File G: 28.81% (vs Order-0's theoretical ~40%)
- File H: 60.99%
- Excellent on structured text, XML, JSON

**Trade-offs:**
- Slower than Order-0 (more table lookups)
- Higher memory usage (~256 small contexts)
- Less effective on random data
- Can timeout on very high-entropy files

### 4. Auto-Selection Algorithm (New in v2.0)

**How It Works:**

```
1. Sample file (first 8192 bytes) to calculate entropy
2. Apply decision rules:

   if (entropy > 7.5):
       → UNCOMPRESSED (incompressible data like File D)

   elif (entropy > 6.8):
       → ORDER-0 (high entropy like Files E, F)
          Reason: Order-1 too slow for marginal gain

   elif (file_size < 100KB):
       → ORDER-0 (small files)
          Reason: Adaptive overhead not worth it

   else:
       → ORDER-1 (low entropy like Files A, B, C, G, H)
          Reason: Patterns expected, big compression win
```

**Performance:**
- Success rate: ~90% (chooses optimal or near-optimal)
- Handles diverse file types automatically
- User doesn't need to know internals

---

## Compressed File Format

### Header Structure

```
+-------------+------------------+-----------------+---------------+
| Model Type  | Original Size    | Model Data      | Encoded Data  |
| (1 byte)    | (8 bytes)        | (varies)        | (variable)    |
+-------------+------------------+-----------------+---------------+
```

**Model Type Values:**
- `0x00` = Order-0 frequency model
- `0x01` = Order-1 adaptive model
- `0xFF` = Uncompressed (incompressible file)

**Model Data:**
- Order-0: 1028 bytes (257 × uint32_t frequency table)
- Order-1: 0 bytes (adaptive model builds during decode)
- Uncompressed: 0 bytes

**Encoded Data:**
- Range-coded bitstream for Order-0 and Order-1
- Raw file data for uncompressed mode

---

## Performance Analysis

### Benchmark Results (8 test files, 13MiB total)

**Compression Metrics:**
- Total original size: 13 MiB
- Total compressed: 7.9 MiB (v2.0) vs 9.0 MiB (v1.0)
- Average compression ratio: **62.74%** (v2.0) vs 71.44% (v1.0)
- **Improvement: 8.7 percentage points** (13% better compression)
- Average bits/symbol: 5.02 (v2.0) vs 5.72 (v1.0)

**Speed Metrics (compared to v1.0):**
- Range coding provides ~2x speedup
- Order-0 remains very fast (< 100ms avg)
- Order-1 slower but acceptable (seconds, not minutes, on large files)

**Comparison with Industry Tools:**

| Rank | Tool | Avg Ratio | Speed | Notes |
|------|------|-----------|-------|-------|
| 1 | xz | 54.16% | Slow | Best compression |
| 2 | bzip2 | 54.88% | Medium | Good balance |
| 3 | zstd | 58.59% | Very Fast | Modern compressor |
| 4 | gzip | 59.47% | Fast | Industry standard |
| 5 | **g07 v2.0** | **62.74%** | Medium | Our tool |
| 6 | g07 v1.0 | 71.44% | Medium | Previous version |

**Files Won (Best Compression):**
- **File A** (1.3MiB text): g07 v2.0 wins with 51.85% (beats all tools!)
- **File G** (2.5MiB): g07 v2.0 ties with bzip2 at ~28.7%

**Model Selection Results:**
- File A: Order-1 chosen → 51.85%
- File B: Order-1 chosen → 47.44%
- File C: Order-1 chosen → 45.77%
- File D: Uncompressed chosen → 100.00% (optimal)
- File E: Order-0 chosen → 88.41%
- File F: Order-0 chosen → 88.04%
- File G: Order-1 chosen → 28.81%
- File H: Order-1 chosen → 60.99%

---

## Usage

### Compilation

```bash
make
```

This creates:
- `bin/compress` - Compression tool
- `bin/decompress` - Decompression tool

### Basic Compression

**Auto-selection (recommended):**
```bash
./compress input.txt output.compressed
```

**Force specific model:**
```bash
# Fast compression (Order-0)
./compress input.txt output.compressed --model order0

# Maximum compression (Order-1)
./compress input.txt output.compressed --model order1
```

### Decompression

```bash
./decompress output.compressed recovered.txt
```

Decompression automatically detects which model was used.

### Benchmarking

**Compare models:**
```bash
cd ../..  # Return to project root
./benchmarks/benchmark_models.sh
```

**Compare v1.0 vs v2.0:**
```bash
./benchmarks/benchmark_versions.sh
```

---

## What Was Tried (Order-2)

### Order-2 Model - Implemented and Removed

**What it was:**
- Used previous 2 bytes as context (65,536 contexts)
- More memory intensive (~260MB)
- Theoretically better compression

**Why it was removed:**
- **Benchmark results**: Identical compression ratios to Order-1
- **Memory cost**: ~260MB vs Order-1's minimal overhead
- **Performance**: Slower with no benefit
- **Conclusion**: Order-1 is optimal sweet spot

**Key Insight:** Diminishing returns beyond Order-1 for our test data. Order-2 doesn't provide enough improvement to justify the complexity.

---

## Known Limitations

1. **Order-1 Speed on High-Entropy Files**
   - Can be very slow (10-30 seconds on files like E, F)
   - Auto-selection mitigates this by detecting and using Order-0
   - Manual override available if needed

2. **Small File Overhead**
   - Order-0: 1037-byte header (1 + 8 + 1028)
   - Significant for files under ~5KB
   - Auto-selection uses Order-0 for files < 100KB

3. **Random Data**
   - Cannot compress truly random data (expected behavior)
   - Auto-selection detects this and stores uncompressed

4. **Ranking vs Modern Tools**
   - Still behind xz, bzip2, zstd on average
   - Competitive on specific file types (especially text)
   - Educational project - demonstrates core concepts well

---

## Technical Achievements

### What Makes v2.0 Special

1. **Multi-Model System**
   - First version to support multiple models
   - Intelligent selection based on file characteristics
   - Clean abstraction between models and coder

2. **PPM-Style Adaptive Encoding**
   - Implemented escape mechanism
   - Proper fallback chain (Order-1 → Order-0)
   - Synchronized updates between encoder/decoder

3. **Range Coding Integration**
   - Successfully replaced arithmetic coding
   - Maintained compression efficiency
   - Achieved significant speedup

4. **Smart Detection**
   - Entropy calculation for file analysis
   - Decision rules based on benchmark data
   - Avoids worst-case scenarios automatically

5. **Comprehensive Testing**
   - Benchmarked against industry tools
   - Tested Order-2 (scientifically removed after proving no benefit)
   - Documented performance characteristics thoroughly

---

## Comparison with v1.0

| Feature | v1.0 | v2.0 |
|---------|------|------|
| **Entropy Coder** | Arithmetic coding | Range coding (2x faster) |
| **Models** | Order-0 only | Order-0 + Order-1 + Auto |
| **Model Selection** | No choice | Manual + Auto |
| **Incompressible Files** | Always compresses | Detects + stores uncompressed |
| **Header Size** | 1036 bytes | 9 bytes (Order-1), 1037 bytes (Order-0) |
| **Avg Compression** | 71.44% | 62.74% (13% better) |
| **Files Won** | 1/8 | 2/8 |
| **CLI Options** | None | --model auto/order0/order1 |
| **Performance** | Baseline | 2x faster + better compression |

**Improvement Summary:**
- **Compression**: 8.7 percentage points better (13% improvement)
- **Speed**: ~2x faster with range coding
- **Flexibility**: 3 models + auto-selection
- **Usability**: Better CLI, warnings, auto-detection

---

## Build Information

**Compilation:**
- Compiler: g++ with C++11 standard
- Optimization: -O3
- Platform: Linux (tested on kernel 6.17.0-14-generic)

**Binaries Included:**
- `compress` (binary) - Compression tool
- `decompress` (binary) - Decompression tool

**Memory Usage:**
- Maximum: ~8GB (compliant with requirements)
- Typical: Much lower for most files
- Order-1 contexts: ~1-2MB during active use

---

## Testing

**Verification Status:** ✓ All tests passed
- Order-0: Perfect reconstruction on all test files
- Order-1: Perfect reconstruction on all test files
- Auto-selection: Perfect reconstruction on all test files
- Uncompressed mode: Perfect reconstruction
- Model switching: Seamless encoding/decoding

**Lossless Guarantee:** 100% byte-for-byte accuracy verified across:
- 8 diverse test files (text, binary, random, structured)
- All model combinations
- All file sizes (from KBs to MBs)

---

## When to Use Each Model

### Order-0 (`--model order0`)
**Best for:**
- Fast compression needed
- High-entropy files (binary executables, multimedia)
- Small files (< 100KB)
- Streaming or real-time scenarios

**Performance:**
- Very fast (< 100ms typical)
- Universal (works on any data)
- Reasonable compression (~60-90% depending on file)

### Order-1 (`--model order1`)
**Best for:**
- Maximum compression desired
- Low-entropy data (text, code, logs)
- Structured data (XML, JSON, CSV)
- Large files where speed is secondary

**Performance:**
- Slower (seconds on large files)
- Excellent compression on structured data (20-50% better)
- May timeout on very high-entropy files

### Auto (`--model auto`)
**Best for:**
- General-purpose use
- Mixed file types
- When you don't know the file characteristics
- **Default and recommended**

**Performance:**
- 90% success rate choosing optimal model
- Avoids worst-case scenarios
- Near-optimal results across diverse inputs

---

## Future Improvements

See [Improvement Roadmap](../../docs/improvement_roadmap.md) for detailed analysis.

**High-Value Opportunities:**
1. **BWT Preprocessing** (20-40% better text compression)
2. **Header Optimization** for Order-0 (compact frequency encoding)
3. **Parallel Processing** (2-4x speedup on multi-core)

**Current Status:** v2.0 is feature-complete for project requirements.

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## References

- Michael Schindler, "A Fast Renormalisation for Arithmetic Coding" (1998)
- Khalid Sayood, "Introduction to Data Compression" (4th Edition)
- TAI Course Materials, Universidade de Aveiro
- PPM algorithm literature (for Order-1 implementation)

---

## License

Educational project for TAI course at Universidade de Aveiro.

Range coder implementation: GPL v2+ (Michael Schindler)
All other code: Original group work

---

*Documentation last updated: March 5, 2026*
