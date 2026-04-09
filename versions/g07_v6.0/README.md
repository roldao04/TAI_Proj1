# Version 6.1 - Multi-Order PPM with Static Weight Training

**Date:** April 6, 2026

**Status:** ❌ Experimental - Failed to beat v5.0

---

## Overview

Version 6.1 explores **multi-order PPM with context mixing** using a static weight training approach. Unlike PAQ's true adaptive mixing, this implementation trains mixer weights on a 100KB sample, freezes them, stores them in the file header, and uses identical static weights during both compression and decompression.

**Result:** Failed to improve compression over v5.0. Average ratio: **59.85%** (v5.0: 54.92%). Lost all 8/8 benchmark files by 4.93pp on average while being 157× slower.

**Key Finding:** The hypothesis that "BWT destroys PPM context patterns" was **incorrect**. Enabling BWT+MTF improved compression from 67.94% to 59.85% (8pp gain), proving that BWT preprocessing actually helps multi-order PPM, even though it disrupts sequential context.

---

## Architecture

### Pipeline
```
Input → BWT → MTF → ZRLE → Multi-Order PPM → Context Mixer → Range Coder → Output
         ↑                        ↑                  ↑
    Preprocessing         5 orders {1,2,3,4,5}   Static weights
```

### Components

**1. Multi-Order PPM**
- Orders: {1, 2, 3, 4, 5}
- Hash table: 8M contexts (~768MB RAM)
- LRU eviction when capacity exceeded
- Each order maintains frequency counts for seen contexts

**2. Context Mixer**
- **Static weight training:** Analyze first 100KB → train weights → freeze
- Fixed-point arithmetic (65536 scale) for deterministic encoder/decoder sync
- Weights stored in 32-byte file header
- No runtime updates (unlike PAQ which updates after every bit)

**3. BWT+MTF+ZRLE Preprocessing**
- Same as v5.0: BWT transforms, MTF move-to-front, ZRLE zero-run encoding
- Initially disabled based on flawed "BWT destroys context" hypothesis
- Re-enabled after discovering it improves compression by 8pp

**4. Range Coder**
- Byte-level arithmetic coding (not bit-level like PAQ)
- Encodes one symbol at a time using mixed probability distribution

---

## Configuration

```cpp
struct V6Config {
    std::vector<int> orders = {1, 2, 3, 4, 5};
    size_t hash_table_size = 8 * 1024 * 1024;  // 8M contexts
    size_t max_history = 8 * 1024;              // 8KB history buffer
    double learning_rate = 0.002;               // Training phase only
    bool use_bwt = true;                        // BWT preprocessing
    bool use_mtf = true;                        // MTF transform
    bool use_zrle = true;                       // Zero-run RLE
};
```

**Memory usage:** ~768MB per compression (8M × 96 bytes/entry)

---

## Benchmark Results

**Test corpus:** 8 files (13 MiB total)

| File | Original | v5.0 Ratio | v5.0 Time | v6.1 Ratio | v6.1 Time | Difference | Slowdown |
|------|----------|------------|-----------|------------|-----------|------------|----------|
| A | 1.3 MiB | 53.30% | 91ms | 60.61% | 7.6s | ✗ -7.31pp | 85× |
| B | 1.2 MiB | 17.34% | 55ms | 19.39% | 1.6s | ✗ -2.05pp | 30× |
| C | 2.0 MiB | 25.93% | 103ms | 28.84% | 5.4s | ✗ -2.91pp | 53× |
| D | 2.0 MiB | 100.00% | 10ms | 100.00% | 22.8s | = Tie | 2285× |
| E | 989 KiB | 86.03% | 43ms | 98.86% | 8.9s | ✗ -12.83pp | 207× |
| F | 2.0 MiB | 82.42% | 30ms | 84.38% | 20.0s | ✗ -1.96pp | 665× |
| G | 2.5 MiB | 29.27% | 95ms | 35.43% | 10.1s | ✗ -6.16pp | 107× |
| H | 999 KiB | 44.72% | 87ms | 58.70% | 4.4s | ✗ -13.98pp | 51× |

### Summary

| Metric | v5.0 | v6.1 | Difference |
|--------|------|------|------------|
| **Avg Ratio** | 54.92% ✅ | 59.85% ❌ | +4.93pp worse |
| **Total Compressed** | 6.9 MiB | 7.5 MiB | +0.6 MiB larger |
| **Total Time** | 514 ms | 81.0 sec | 157× slower |
| **Files Won** | 8/8 ✅ | 0/8 ❌ | Total loss |

**v5.0 wins all 8/8 files.**

---

## Analysis: Why v6.1 Failed

### 1. Static Weights Can't Adapt

**Problem:** Weights trained on the first 100KB don't represent the entire file.

- Files have varying statistics across sections (e.g., header vs data vs metadata)
- Static weights optimal for one section are suboptimal for others
- PAQ solves this with **adaptive weights** that update after every bit encoded

**Example:** File A has low-entropy text in the first 100KB but higher-entropy structured data later. Static weights trained on text perform poorly on the structured section.

### 2. Byte-Level Coding Limitation

**Problem:** Range coder encodes one **byte** at a time, leaving 3-7 bits of information on the table per symbol.

- PAQ encodes one **bit** at a time for maximum compression
- Byte-level: best case is 1 bit/symbol, average is 4.78 bits/symbol
- Bit-level: can achieve fractional bits/symbol (e.g., 0.1 bits for highly predictable sequences)

**Impact:** Even with perfect prediction, byte-level coding has an ~8-10% compression penalty vs bit-level.

### 3. Pure PPM Underperforms on Binary Files

**Evidence:**
- File B (genomic): v6.1 loses by 2.05pp
- File H (binary mixed): v6.1 loses by 13.98pp (worst case)
- High-entropy files (E, F): v6.1 expands data more than v5.0

**Root cause:** Multi-order PPM works best on natural text with repeating word patterns. Binary files lack this structure.

- v5.0's BWT creates artificial structure (long runs of similar bytes)
- Pure PPM tries to find sequential patterns that don't exist in binary

### 4. BWT+MTF Actually Helps (Hypothesis Was Wrong)

**Original hypothesis:** "BWT destroys PPM context patterns by scrambling symbol order."

**Test result:** Disabling BWT made compression **worse**.

| Configuration | Avg Ratio | Verdict |
|---------------|-----------|---------|
| **With BWT+MTF+ZRLE** | 59.85% | Better |
| Without BWT/MTF/ZRLE | 67.94% | Worse (-8.09pp) |

**Why BWT helps PPM:**
- BWT clusters similar bytes together (e.g., all 'a's near each other)
- MTF converts repeated symbols to zeros
- ZRLE compresses zero runs
- Result: PPM sees highly predictable byte streams (lots of Order-1 hits)

**Lesson:** Even though BWT disrupts sequential context, it creates **local context** that PPM exploits better than raw binary patterns.

### 5. Hash Table Overflow

**Problem:** With 5 orders, hash table usage approached capacity on large files.

- 8M context limit reached after ~2MB of input
- LRU eviction discards useful long-term context
- Testing with 9 orders {1,2,3,4,5,6,8,12,16} caused table overflow (2400% capacity) and decompression corruption

**Impact:** Higher-order contexts (order 4, 5) frequently evicted before accumulating useful statistics.

---

## Comparison with PAQ

| Feature | v6.1 (This Version) | PAQ (True Adaptive Mixing) |
|---------|---------------------|----------------------------|
| **Weight Updates** | Static (frozen after training) | Adaptive (every bit) |
| **Training** | 100KB sample → freeze | Start equal → adapt continuously |
| **Weight Storage** | 32 bytes in file header | Not needed (deterministic sync) |
| **Coding Level** | Byte-level | Bit-level |
| **Models** | Only PPM orders | PPM + match + sparse + word + ... |
| **Mixing** | Linear weighted average | Logistic (neural-like) |
| **Update Rule** | None (static) | Gradient descent on prediction error |

**Key difference:** PAQ's weights **improve** during compression as the model learns the file's characteristics. v6.1's weights are **fixed** and never adapt.

---

## Development Notes

### Configurations Tested

1. **Original (April 5):** BWT disabled, 5 orders → 67.94% avg
2. **Final (April 6):** BWT enabled, 5 orders → 59.85% avg (documented here)
3. **Failed (April 5):** 9 orders → decompression corruption

### Issues Encountered

- **Hash table overflow:** 9+ orders exceeded 8M capacity, caused corruption
- **Decompression errors:** Orders {8, 12, 16} produced incorrect output
- **Training instability:** Some files produced negative weights (clamped to 0.001 min)

---

## Files Included

- **G07-V6.1-C** — Compressor binary
- **G07-V6.1-D** — Decompressor binary

Both built with:
```bash
make clean && make v6
g++ -std=c++17 -O3 -march=native -flto=auto
```

---

## Recommendations

### Do NOT Use v6.1 for Production

- **4.93pp worse** compression than v5.0
- **157× slower** (81 seconds vs 0.5 seconds on 13 MiB)
- **No advantages** over simpler v5.0 architecture

### Use v5.0 Instead

v5.0 (BWT+MTF+Order-1 PPM) remains superior:
- Better compression (54.92% vs 59.85%)
- Much faster (0.5s vs 81s)
- Simpler implementation
- Competitive with bzip2 (54.93%)

---

## Next Steps (v7.0 Roadmap)

If teacher requires higher compression, three paths forward:

### Option A: True Adaptive Mixing (PAQ-style)
- Remove static weight training
- Update weights after every **byte** encoded (keep byte-level for simplicity)
- Both encoder/decoder start with equal weights, update identically
- No need to store weights in header (deterministic sync)
- **Effort:** 2-3 days
- **Expected:** ~2-4pp improvement over v6.1, still slower than v5.0

### Option B: Bit-Level Arithmetic Coding
- Redesign entire pipeline for bit-level prediction
- Multi-order PPM predicts next **bit**, not byte
- **Effort:** 4-5 days (high risk with deadline)
- **Expected:** 5-8pp improvement, but complex

### Option C: Hybrid v5.0 + Higher-Order PPM
- Keep v5.0's winning BWT+MTF
- Replace Order-1 PPM with adaptive Order-2 or Order-3
- Simpler than v6.1, incremental improvement
- **Effort:** 1-2 days (lowest risk)
- **Expected:** 1-2pp improvement over v5.0

**Recommendation:** Option C (v5.0 + Order-2) or accept v5.0 as final submission (already beats bzip2).

---

## Conclusion

v6.1 demonstrates that **static weight training** is not an effective approach for compression. The inability to adapt weights during compression, combined with byte-level coding and pure PPM's poor performance on binary files, results in worse compression than the much simpler v5.0.

However, the experiment yielded valuable insights:
1. BWT+MTF **helps** multi-order PPM (contrary to initial hypothesis)
2. Static weights can't adapt to file sections
3. True adaptive mixing (PAQ-style) is necessary for competitive results

v6.1 serves as an educational example of what **not** to do in compression algorithm design.

---

*Last updated: 2026-04-06 (v6.1 experimental release)*
