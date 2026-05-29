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
│   ├── arithmetic/                # Range coder, rANS, arithmetic coder
│   ├── model/                     # Frequency / context / PPM / PRNG models
│   ├── transform/                 # BWT, MTF, ZRLE, LZP, x86 filter
│   ├── utils/                     # File I/O, stream header, entropy calc
│   ├── libsais.c                  # Linear-time suffix array (BWT)
│   ├── compressor_v6..v10.cpp     # Per-version compressors
│   └── decompressor_v6..v10.cpp   # Per-version decompressors
├── include/                       # Header files
├── tests/                         # Unit tests (run via `make check`)
├── benchmarks/                    # Benchmark scripts (+ macos/ variant)
├── versions/                      # Historical version binaries + per-version docs
├── report/                        # LaTeX report + figures
├── data/                          # Test data files A–H (not tracked in git)
├── Makefile                       # Build system
└── README.md                      # This file
```

---

## Building

Compile both compressor and decompressor:

```bash
make
```

The build auto-detects every `src/compressor_v*.cpp` and produces:
- `bin/g07-v9-c` / `bin/g07-v9-d` - **Best compression ratio** (per-block candidate search + Order-1/2)
- `bin/g07-v10-c` / `bin/g07-v10-d` - Best ratio **plus PRNG / Kolmogorov detection**
- `bin/g07-v7-c` / `bin/g07-v7-d` - Speed-optimized (BWT + MTF + interleaved rANS)
- `bin/g07-v8-c` / `bin/g07-v8-d` - Ultra-fast (LZ77, no entropy coding)
- `bin/g07-v6-c` / `bin/g07-v6-d` - Multi-order PPM (experimental)

To clean build artifacts:

```bash
make clean
```

---

## Usage

All binaries take `<input_file> <output_file>` — no flags needed (optimal settings are hardcoded).

### Compression

Choose a version based on your priority:

**v9 — Best Compression Ratio (~54.3% avg)**
```bash
./bin/g07-v9-c <input_file> <output_file>
```
- Per-block candidate search (raw / BWT+MTF / LZP / X86 + Order-1/Order-2) + Range Coder
- Best ratio of all versions; beats bzip2/xz/gzip/zstd on Files A, E, G

**v10 — Best Ratio + PRNG / Kolmogorov Detection**
```bash
./bin/g07-v10-c <input_file> <output_file>
```
- Runs the same per-block best-ratio search as v9
- Adds detection of pseudorandom data from glibc `rand()` (seeds 0–65535)
- Compresses PRNG output to **14 bytes** regardless of file size (model id + seed + length)
- Demonstrates the gap between Shannon entropy and Kolmogorov complexity

**v7 — Maximum Speed (~29 MB/s compress, ~51 MB/s decompress)**
```bash
./bin/g07-v7-c <input_file> <output_file>
```
- BWT + MTF + 2-way interleaved rANS Order-0; ~59% avg ratio

**v8 — Ultra-Fast (LZ77, ~200/300 MB/s)**
```bash
./bin/g07-v8-c <input_file> <output_file>
```
- LZ4-style LZ77 dictionary matching, byte-aligned tokens, no entropy coding

**Examples:**
```bash
./bin/g07-v9-c  data/A data/A.compressed   # best ratio
./bin/g07-v7-c  data/A data/A.fast         # best speed
./bin/g07-v10-c data/D data/D.compressed   # PRNG detection (2MB → 14 bytes)
```

### Decompression

Use the decompressor matching the version that produced the file:
```bash
./bin/g07-v9-d  <compressed_file> <output_file>
./bin/g07-v10-d <compressed_file> <output_file>
./bin/g07-v7-d  <compressed_file> <output_file>
./bin/g07-v8-d  <compressed_file> <output_file>
```

Each decompressor reads the model-type byte in the header and auto-detects the pipeline. Streams are not interchangeable across version families (e.g. a v7 rANS file is not readable by `v9-d`); the tool prints a clear error if the wrong decompressor is used.

**Example:**
```bash
./bin/g07-v9-d data/A.compressed data/A.decompressed
./bin/g07-v7-d data/A.fast       data/A.decompressed
```

---

## Verification

Run the unit-test suite (BWT, LZP, context model, x86 filter):

```bash
make check
```

This builds and runs all unit tests; each verifies byte-for-byte correctness
(BWT transform/inverse, block processing, edge cases, LZP, context model, x86 filter).

Run a single-file lossless roundtrip for a specific version:

```bash
make test FILE=data/A VERSION=9   # compress + decompress + verify identical
```

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

**Single file benchmark (G07 vs gzip/bzip2/xz/zstd):**
```bash
./benchmarks/macos/compare.sh <file>
```

By default this uses the v9 binary; override `G07_COMPRESS_CMD` / `G07_DECOMPRESS_CMD`
to benchmark another version. (Despite the folder name, the script also runs on Linux.)

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
  - `13` (0x0D) = PRNG Detected (Kolmogorov compression)
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

### PRNG Detection (Kolmogorov Compression)

File D in the dataset demonstrates a fundamental concept in Algorithmic Information Theory: data can have **maximum Shannon entropy** (appear perfectly random to any statistical test) while having **minimal Kolmogorov complexity** (be trivially generated by a short program).

**The problem:** File D has 7.999922 bits/byte Shannon entropy — virtually the theoretical maximum of 8.0. Every standard compressor (gzip, bzip2, xz, zstd) and every statistical model (Order-0, Order-1, BWT+MTF, LZ77) stores it uncompressed because no byte-frequency biases, repeated substrings, or context correlations exist.

**The insight:** File D is the output of glibc's `rand()` function seeded with 1 — a deterministic pseudorandom number generator. The entire 2,000,000-byte file can be reproduced by:
```c
srand(1);
for (int i = 0; i < 2000000; i++)
    putchar(rand() & 0xFF);
```

**The algorithm:** The glibc TYPE_3 PRNG uses an additive feedback generator with a degree-31 trinomial (x^31 + x^3 + 1):
- **Initialization:** `state[0] = seed; state[i] = (16807 * state[i-1]) mod (2^31 - 1)` for i = 1..30
- **Warm-up:** 310 iterations before first output
- **Recurrence:** `state[fptr] = (state[fptr] + state[rptr]) mod 2^32`, with fptr and rptr 3 positions apart in a 31-element circular buffer
- **Output:** `state[fptr] >> 1` (31-bit positive integer); the file stores only the low byte (`& 0xFF`)

**Detection method:** Brute-force seed search (seeds 0–65535) with two-phase matching:
1. Generate 64 prefix bytes for each candidate seed; compare with input
2. On prefix match, verify the full file (probability of false prefix match: ~(1/256)^64 ≈ 0)

**Compressed format:** `[0x0D][Original Size: 8B][Algorithm ID: 1B][Seed: 4B]` = **14 bytes total**

**Result:** 2,000,000 bytes → 14 bytes (compression ratio: 0.0007%, ratio 142,857:1)

| Metric | Value |
|--------|-------|
| Shannon entropy H(X) | 7.999922 bits/byte |
| Kolmogorov complexity K(X) | ~14 bytes |
| Standard compressor output | ~2,000,009 bytes (expansion) |
| PRNG model output | **14 bytes** |

This is implemented in `src/model/prng_model.cpp` and integrated into the v10 compressor (v10 = v9's best-ratio pipeline plus the PRNG detector).

---

## Performance Notes

- **Memory**: Uses ~1-2GB (requirement compliant)
- **Optimization**: Compiled with `-O3 -march=native -flto=auto` for maximum performance
- **Platform**: Tested on Linux
- **Binary Size**: < 120KB per production binary (well under 1MB requirement)

### Production Versions

| Metric | v9 (Best Ratio) | v10 (Best Ratio + PRNG) | v7 (Best Speed) | v8 (Ultra-Fast) |
|--------|-----------------|-------------------------|-----------------|-----------------|
| **Avg Compression Ratio** | **~54.3%** | ~54.3% (0.0007% on PRNG) | 59.2% | 38–88% (compressible) |
| **Compress Speed** | ~1.5 MB/s | ~1.5 MB/s (+ PRNG scan) | **28.9 MB/s** | **~200 MB/s** |
| **Decompress Speed** | ~18 MB/s | ~18 MB/s (instant for PRNG) | 51.1 MB/s | **~300 MB/s** |
| **Pipeline** | Per-block candidate search + Order-1/2 | v9 search + PRNG detector | BWT+MTF+interleaved rANS | LZ77 (no entropy coding) |
| **Best For** | Archival, max compression | AIT demo, PRNG data | Real-time, throughput | Streaming, lowest latency |

*v5 (BWT+MTF+ZRLE+Order-1, 54.7% avg) was the previous best-ratio version; it is superseded by v9 and kept as a prebuilt binary in `versions/g07_v5.0/`.*

---

## Version History

See [versions/VERSIONS.md](versions/VERSIONS.md) for detailed version history.

**v10 (April 2026) - Kolmogorov Compression**
- v9's best-ratio pipeline plus glibc `rand()` PRNG detection (seeds 0–65535)
- **0.0007% ratio** on PRNG data (2MB → 14 bytes); demonstrates Shannon entropy vs Kolmogorov complexity
- Lossless on all 8 test files

**v9 (April 2026) - Best Ratio (current)**
- Per-block multi-pipeline candidate search (raw / BWT+MTF / LZP / X86 + Order-1/2)
- **~54.3% avg ratio** — best of all versions; wins Files A, E, G vs bzip2/xz/gzip/zstd
- Supersedes v5 as the compression-optimized build

**v8 (April 2026) - Ultra-Fast**
- LZ4-style LZ77 dictionary matching, byte-aligned tokens, no entropy coding
- ~200 MB/s compress, ~300 MB/s decompress; smallest binaries (~33KB)

**v7 (April 2026) - Speed-Optimized**
- BWT + MTF + 2-way interleaved rANS Order-0
- **59.2% avg ratio** at **51.1 MB/s decompression**

**Earlier versions** (superseded; kept as prebuilt binaries in `versions/`):
- **v5.0 / v5.x**: BWT+MTF+ZRLE+Order-1 adaptive PPM + Range Coding — 54.7% (predecessor of v9)
- **v6.1**: Multi-order PPM + context mixing — failed experiment (59.85%, 157× slower)
- **v4.0**: Flat-array context model + libsais + parallel BWT — 56.39%
- **v3.0**: BWT preprocessing introduced — 57.48%
- **v2.0**: Multi-model + Range coding — 62.74%
- **v1.0**: Order-0 + Arithmetic coding — 71.44%

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
- PRNG model: glibc TYPE_3 rand() algorithm (GNU C Library, `stdlib/random_r.c`), degree-31 trinomial from Knuth TAOCP Vol. 2
- Kolmogorov complexity concepts: Based on Algorithmic Information Theory (Kolmogorov, Solomonoff, Chaitin)

---

## Authors

- Guilherme Rosa (113968)
- João Roldão (113920)
- Henrique Teixeira (114588)

---

## License

Educational project for TAI course at Universidade de Aveiro.
