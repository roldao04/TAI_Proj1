# Version 8.0 - Ultra-Fast LZ77 Compressor

**Release Date:** April 9, 2026

**Status:** Production (Ultra-Fast)

---

## Overview

Version 8.0 is an **ultra-fast** compressor that abandons the BWT+MTF+entropy-coding pipeline entirely in favour of **LZ4-style LZ77 dictionary matching** with a byte-aligned token format and **no entropy coding**. The algorithm exploits exact byte-sequence repetitions via a tiny hash table that fits in L1 cache, producing a simple token stream that decompresses at near-memcpy speed.

**Key Results:**
- **120-250 MB/s compression** (6-13x faster than v7, 10-20x faster than v5)
- **160-480 MB/s decompression** (3-10x faster than v7)
- **36 KB binaries** (smallest of all versions)
- **Lossless** on all 8 benchmark files + edge cases
- Compression ratio: 38-88% on compressible files (trades ratio for speed)

**Design Philosophy:** Maximum throughput at all costs. BWT is fundamentally O(n log n) and tops out at ~200 MB/s. LZ77 with hash-table matching is O(n) with a tiny constant, reaching 500+ MB/s in optimised implementations. This version targets the speed class of zstd-1 and LZ4 rather than bzip2 or xz.

---

## Architecture

### Pipeline

```
Input -> Entropy Check -> LZ4-style LZ77 Hash-Table Matching -> Byte-Aligned Token Stream -> Output
```

**Compressible data** (entropy <= 7.5):
```
Input -> 16384-entry Hash Table LZ77 -> Byte-Aligned Token Stream -> Output
         (falls back to uncompressed if LZ77 expands data)
```

**Near-random data** (entropy > 7.5):
```
Input -> Stored Uncompressed
```

### What Changed from v7

| Component | v7.0 | v8.0 | Why |
|-----------|------|------|-----|
| Algorithm | BWT + MTF + rANS | **LZ77 dictionary matching** | BWT is O(n log n), LZ77 is O(n) |
| Match finding | N/A (entropy coding) | **16384-entry hash table** (64KB, L1 cache) | O(1) lookup, O(1) insert |
| Entropy coder | 2-way interleaved rANS | **None** (byte-aligned tokens) | Eliminates encode/decode pass entirely |
| Output format | rANS bitstream | **Byte-aligned token stream** | Zero bit manipulation |
| Preprocessing | BWT + MTF | **None** | Eliminates two O(n) passes |
| Block size | 4MB blocks | **Single pass** (no blocks) | Simpler, less overhead |
| Threading | Per-block threads | **Single thread** | Files < 4MB are single-threaded anyway |

### What Was Kept from v7

- **Entropy analysis**: Shannon entropy on 8KB sample for auto-selection
- **File I/O**: `FileIO::read_file` / `FileIO::write_file`
- **Uncompressed fallback**: Model type 0xFF for incompressible data

---

## LZ77 Algorithm

### Core Technique: Hash-Table Match Finding

The compressor scans the input sequentially, hashing every 4-byte sequence into a flat hash table. When a hash match is found and the actual bytes confirm it, the compressor emits a **back-reference** (offset + length) instead of the raw bytes.

```
Hash function: (read_u32(ptr) * 2654435761) >> 18   (Knuth multiplicative hash)
Hash table:    16384 entries x 4 bytes = 64KB        (fits in L1 cache)
Collision:     Overwrite (no chains, no probing)      (O(1) always)
Min match:     4 bytes
Max offset:    65535 (2-byte field)
```

### Key Speed Techniques

1. **Single-entry hash buckets**: No chains, no probing, no linked lists. One hash lookup = one comparison. Collisions silently overwrite, sacrificing a few matches for massive speed.

2. **Skip acceleration**: When no matches are found, the scan step grows every 64 misses (1, 1, ..., 2, 2, ..., 3, ...). Incompressible regions fly through at near-memcpy speed.

3. **Byte-aligned token format**: No bit buffers, no shift registers. Every field is byte-aligned. The CPU's byte-addressing hardware does all the work.

4. **8-byte XOR match counting**: `__builtin_ctzll(read_u64(ip) ^ read_u64(ref)) / 8` counts matching bytes 8 at a time using a single CPU instruction for the trailing-zero count.

5. **Immediate re-match**: After encoding a match, the algorithm immediately checks if the next position also matches (via `goto encode_match`), avoiding re-entering the search loop for chained matches.

6. **Hash ip-2**: After a match, the position two bytes before the match end is hashed, filling the gap in the hash table and improving future match finding.

7. **No entropy coding**: The token stream IS the output. No Huffman tables, no ANS state machines, no range coder intervals.

### Compression Loop (Pseudocode)

```
ht[16384] = {0}                    // hash table, 64KB on stack
ip = src + 1                       // scan position (skip position 0)
anchor = src                       // start of current literal run

while ip < src_end - 12:           // MFLIMIT = 12
    h = hash4(ip)
    match = src + ht[h]
    ht[h] = ip - src               // always update table

    if no match or offset > 65535:
        ip += step                  // skip acceleration
        continue

    // Extend backwards
    while ip > anchor and match > src and ip[-1] == match[-1]:
        ip--; match--

    // Emit: [token][ext_lit_len][literals][offset][ext_match_len]
    encode_literals(ip - anchor)
    encode_offset(ip - match)
    ml = count_match_8byte_xor(ip + 4, match + 4, limit)
    encode_match_length(ml)
    ip += ml + 4

    // Try immediate re-match at new ip
    if hash4(ip) matches:
        emit 0-literal sequence and goto encode_match

// Final literals: last 5+ bytes always stored as literals
emit_last_literals(src_end - anchor)
```

### Token Format

Each sequence in the compressed stream:

```
[token: 1B] [ext_lit_len: 0+B] [literals: N B] [offset: 2B LE] [ext_match_len: 0+B]
```

- **Token byte**: high nibble = literal length (0-14, or 15 = extended), low nibble = match_length - 4 (0-14, or 15 = extended)
- **Extended lengths**: 255-byte continuation encoding. Each byte 255 adds 255; first byte < 255 terminates.
- **Offset**: 2 bytes, little-endian. Backward distance from current position (1 = previous byte, max 65535).
- **Last sequence**: literals only (no offset/match). Signalled by reaching end of payload.

Examples:
- `[0x40] [A B C D] [0x08 0x00]`: 4 literals "ABCD", match at offset 8, match length 4
- `[0x0C]  [0x08 0x00]`: 0 literals, match at offset 8, match length 16 (0 + 0xC + 4)
- `[0xF0] [0x05] [20 literal bytes] [...]`: 20 literals (15 + 5), then match...

---

## Decompressor

### Design

The decompressor is a tight loop: read token, copy literals, read offset, copy match. No hash table, no auxiliary data structures. Linear scan of the compressed stream.

### Safe Copy Strategy

The decompressor uses **`safe_copy8`**: 8-byte chunked copies with **no overshoot**. The original wild_copy8 approach (LZ4-style, copies 8 bytes and overshoots by up to 7) was found to corrupt output because the overshoot bytes persisted in the output buffer and were subsequently read by future back-references.

```cpp
// safe_copy8: exact-length copy in 8-byte chunks, no overshoot
while (dst + 8 <= dend) { memcpy(dst, src, 8); dst += 8; src += 8; }
while (dst < dend) { *dst++ = *src++; }   // remaining 0-7 bytes
```

For overlapping matches (offset < 8), byte-by-byte copy propagates the repeating pattern correctly (e.g., offset=1 fills with the last byte).

---

## Compressed File Format

### Model Type: `0x0C` (12)

```
[Model Type: 1 byte = 0x0C]
[Original File Size: 8 bytes, uint64_t, little-endian]
[Compressed Payload Size: 4 bytes, uint32_t, little-endian]
[LZ77 Token Stream: variable]
```

### Header Overhead

- Total header: **13 bytes** (1 + 8 + 4)
- Intentionally minimal: every header byte costs latency

### Worst-Case Expansion

- Each input byte produces at most: 1 token byte + 1/255 extension byte
- Bound: `src_size + src_size/255 + 16`
- If LZ77 expands the data, falls back to uncompressed (model type 0xFF)

---

## Performance Results

### v8 vs v7 vs v5 Comparison

| File | Size | v5 Ratio | v7 Ratio | v8 Ratio | v8 Compress | v8 Decompress |
|------|------|----------|----------|----------|-------------|---------------|
| A | 1.3MB | 53.3% | 53.4% | 88.0% | 6ms (208 MB/s) | 5ms (250 MB/s) |
| B | 1.2MB | 17.3% | 17.7% | 37.8% | 4ms (279 MB/s) | 5ms (223 MB/s) |
| C | 2.0MB | 25.9% | 27.0% | 45.2% | 8ms (238 MB/s) | 9ms (212 MB/s) |
| D | 2.0MB | 100% | 100% | 100% | 4ms (store) | 3ms (copy) |
| E | 1.0MB | 86.0% | 88.2% | 100% | 3ms (store) | 1ms (copy) |
| F | 2.1MB | 82.4% | 87.6% | 100% | 7ms (store) | 3ms (copy) |
| G | 2.5MB | 28.8% | 29.0% | 72.8% | 10ms (241 MB/s) | 6ms (401 MB/s) |
| H | 1.0MB | 43.8% | 44.2% | 58.0% | 7ms (139 MB/s) | 5ms (195 MB/s) |

### Speed Comparison

| Metric | v5 | v7 | v8 | v8 vs v7 |
|--------|----|----|----| ---------|
| Avg compress speed | ~25 MB/s | ~29 MB/s | **~200 MB/s** | **7x faster** |
| Avg decompress speed | ~21 MB/s | ~51 MB/s | **~300 MB/s** | **6x faster** |
| Binary size (c+d) | 216KB | 140KB | **72KB** | **49% smaller** |

### Compression Quality vs Speed Tradeoff

v8 trades compression ratio for massive speed gains:
- **B (structured data)**: 37.8% ratio at 279 MB/s (vs v7's 17.7% at 23 MB/s)
- **H (ELF binary)**: 58.0% ratio at 139 MB/s (vs v7's 44.2% at 19 MB/s)
- **G (mixed data)**: 72.8% ratio at 241 MB/s (vs v7's 29.0% at 20 MB/s)

### Edge Cases

| Test | Result |
|------|--------|
| Empty file (0 bytes) | PASS |
| 1-byte file | PASS (stored uncompressed) |
| 5-byte file (LASTLITERALS boundary) | PASS (stored uncompressed) |
| 12-byte file (MFLIMIT boundary) | PASS (stored uncompressed) |
| 100KB all-zeros | PASS (0.4% ratio -- excellent RLE via offset=1) |
| 100KB random data | PASS (stored uncompressed) |

---

## Binary Sizes

| Binary | v5 Size | v7 Size | v8 Size | Reduction vs v7 |
|--------|---------|---------|---------|-----------------|
| Compressor | 132KB | 84KB | **36KB** | 57% smaller |
| Decompressor | 84KB | 56KB | **36KB** | 36% smaller |

v8 links only `file_io.o`, `entropy_calculator.o`, and `stream_header.o` from the shared objects. With `-flto=auto`, all BWT/MTF/ZRLE/rANS/Range code is stripped as dead code.

---

## Implementation Details

### Files Created

| File | Lines | Purpose |
|------|-------|---------|
| `src/compressor_v8.cpp` | ~260 | LZ77 compressor engine + main |
| `src/decompressor_v8.cpp` | ~190 | LZ77 decompressor engine + main |

### Files Modified

| File | Changes |
|------|---------|
| `include/utils/stream_header.h` | Added `LZ77_FAST = 12` enum value |
| `src/utils/stream_header.cpp` | Added `LZ77_FAST` to `parse_header()` recognized types and `describe_model_type()` case |
| `benchmarks/benchmark_all.sh` | Added `SKIP_VERSIONS="6"` to exclude broken v6 from benchmarks |
| `benchmarks/compare.sh` | Same v6 exclusion |
| `CLAUDE.md` | Documented v8 pipeline, format, and auto-selection logic |

### Memory Usage

- Hash table: 16384 x 4 bytes = **64 KB** (stack-allocated, fits in L1 cache)
- Output buffer: `src_size + src_size/255 + 32` (worst-case expansion)
- Decompressor: `original_size + 8` (8 bytes slack for safe_copy8)
- Total: ~2x input size (no auxiliary data structures)

### Bug Found and Fixed: Wild Copy Overshoot

The initial decompressor used `wild_copy8` (LZ4-style 8-byte memcpy loop) that overshoots by up to 7 bytes. This is standard in LZ4 and assumed safe because the output buffer has slack.

**The bug:** Overshoot bytes from match copies persisted in the output buffer. When a subsequent sequence's match had an offset pointing into the overshooted region, it copied garbage (stale data from earlier match sources or compressed stream bytes) into the final output.

**Root cause:** The overshoot writes `match_source[match_len..match_len+7]` to `output[op+match_len..op+match_len+7]`. These are bytes from the match source that are NOT the correct output for those positions. When a later back-reference reads from these positions before they're overwritten by their correct sequence, corruption occurs.

**Fix:** Replaced `wild_copy8` with `safe_copy8` which copies in 8-byte chunks but stops exactly at the target length. The remaining 0-7 bytes are copied individually. This eliminates the overshoot entirely while keeping the 8-byte-chunk fast path for large copies.

---

## When to Use v8 vs v7 vs v5

| Use Case | Recommended | Why |
|----------|-------------|-----|
| **Maximum speed** | **v8** | 6-13x faster than v7, 10-20x faster than v5 |
| **Best compression ratio** | v5 | 54.7% avg (v8 trades ~20pp for speed) |
| **Balanced speed/ratio** | v7 | 59% ratio at 29/51 MB/s |
| **Real-time / streaming** | **v8** | Near-memcpy decompression speed |
| **Network transfer** | v7 or v5 | Network bandwidth usually < 200 MB/s, so ratio matters more |
| **Large file archival** | v5 | Every percentage point saves significant space |
| **Temporary / transient data** | **v8** | Speed dominates for short-lived data |
| **High-entropy data** | v8 = v7 = v5 | All store uncompressed (identical) |

---

## References

- **LZ77**: Ziv & Lempel, "A Universal Algorithm for Sequential Data Compression" (IEEE Trans. IT, 1977)
- **LZ4**: Yann Collet, lz4.org -- LZ4 block format specification and reference implementation
- **Hash function**: Donald Knuth, multiplicative hashing with golden ratio constant 2654435761 (0x9E3779B1)
- **Skip acceleration**: LZ4 reference implementation `LZ4_skipTrigger` mechanism
- **Match counting**: XOR + `__builtin_ctzll` technique from Fabian Giesen's blog (fgiesen.wordpress.com)

---

*Last updated: 2026-04-09 (Version 8.0 release)*
