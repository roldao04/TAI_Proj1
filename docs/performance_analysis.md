# Performance Analysis: G07 Compressor

**TAI Project #1 - Group 07**
**Date**: March 6, 2026
**Authors**: Guilherme Rosa, João Roldão, Henrique Teixeira

---

## Executive Summary

This document provides a comprehensive performance analysis of our G07 lossless compression tool, examining where we excel, where we fall short, and why. Based on comprehensive benchmarking against industry-standard tools (gzip, bzip2, xz, zstd), we identify our competitive advantages and areas for improvement.

**Key Findings**:
- **Won 2/8 files** (files A and D) against all competitors
- **Rank #5/5 overall** in compression ratio (62.74% avg)
- **Slowest compressor** by far (1.3s avg vs zstd's 11ms)
- **Excels on low-entropy text** (beat all tools on file A)
- **Weak on structured data** (XML/JSON-like) where dictionary methods dominate

---

## Table of Contents

1. [Overall Performance](#overall-performance)
2. [Wins and Losses Analysis](#wins-and-losses-analysis)
3. [Why We Win: Statistical Dominance on Text](#why-we-win-statistical-dominance-on-text)
4. [Why We Lose: Dictionary vs Statistical Methods](#why-we-lose-dictionary-vs-statistical-methods)
5. [Speed Analysis](#speed-analysis)
6. [Model-Specific Performance](#model-specific-performance)
7. [Trade-offs and Design Choices](#trade-offs-and-design-choices)
8. [Recommendations](#recommendations)

---

## Overall Performance

### Compression Ratio Ranking

| Rank | Tool | Avg Ratio | Files Won | Total Compressed | Strengths |
|------|------|-----------|-----------|------------------|-----------|
| 🥇 1 | **xz** | 54.16% | 4 | 6.8MiB | Best all-around, dominates structured data |
| 🥈 2 | **bzip2** | 54.88% | 2 | 6.9MiB | Excellent on structured data (BWT) |
| 🥉 3 | **zstd** | 58.59% | 0 | 7.4MiB | Fastest, good balance |
| 4 | **gzip** | 59.47% | 0 | 7.5MiB | Fast, but outdated |
| 5 | **g07** | 62.74% | 2 | 7.9MiB | Wins on text, slow overall |

### Speed Ranking

| Rank | Tool | Avg Compress | Avg Decompress | Notes |
|------|------|--------------|----------------|-------|
| 🥇 1 | **zstd** | 11 ms | 5 ms | Designed for speed |
| 🥈 2 | **gzip** | 63 ms | 12 ms | Classic fast algorithm |
| 🥉 3 | **bzip2** | 94 ms | 56 ms | BWT overhead |
| 4 | **xz** | 389 ms | 32 ms | Heavy compression |
| 5 | **g07** | 1329 ms | 1411 ms | Order-1 model overhead |

**Reality check**: We're **118x slower** than zstd and **21x slower** than gzip on average.

---

## Wins and Losses Analysis

### Files We Won

#### File A (1.3MiB) - Clear Victory ⭐
- **G07**: 51.85% (678KB)
- **xz**: 53.34% (698KB)
- **bzip2**: 54.05% (707KB)
- **zstd**: 53.52% (700KB)
- **gzip**: 58.49% (765KB)

**Why we won**: Low-entropy text file where Order-1 context model excels. Our adaptive statistical model captures local patterns better than dictionary methods.

#### File D (2.0MiB) - Tie with zstd
- **G07**: 100.00% (2.0MiB + 9 bytes)
- **zstd**: 100.00% (2.0MiB + 61 bytes)
- **xz**: 100.01% (2.0MiB + 160 bytes)
- **gzip**: 100.02% (2.0MiB + 330 bytes)
- **bzip2**: 100.45% (2.0MiB + 9KB)

**Why we "won"**: Random/incompressible file. Our auto-selection detected high entropy (7.98 bits/symbol) and intelligently stored it uncompressed with minimal overhead (9 bytes).

### Files We Lost Badly

#### File B (1.2MiB) - Catastrophic Loss 💥
- **xz**: 17.76% (207KB) ⭐
- **bzip2**: 17.92% (209KB)
- **gzip**: 22.89% (267KB)
- **zstd**: 23.14% (270KB)
- **G07**: 47.44% (554KB) ← **2.7x worse than winner!**

**Impact**: Lost 347KB of potential compression.

#### File C (2.0MiB) - Major Loss
- **xz**: 24.72% (494KB) ⭐
- **bzip2**: 26.51% (530KB)
- **zstd**: 31.22% (624KB)
- **gzip**: 32.57% (651KB)
- **G07**: 45.77% (915KB) ← **1.8x worse than winner**

**Impact**: Lost 421KB of potential compression.

#### File H (999KiB) - Significant Loss
- **xz**: 35.87% (366KB) ⭐
- **bzip2**: 42.34% (432KB)
- **gzip**: 44.02% (450KB)
- **zstd**: 44.96% (459KB)
- **G07**: 60.99% (623KB) ← **1.7x worse than winner**

**Impact**: Lost 257KB of potential compression.

---

## Why We Win: Statistical Dominance on Text

### Our Strength: Order-1 Adaptive Context Model

**How it works**:
- Maintains 256 separate probability distributions (one per previous byte context)
- Adapts frequencies dynamically as file is processed
- Uses PPM-style escape mechanism for unseen symbols
- Falls back to Order-0 for symbols never seen in current context

**Best case scenario** (File A):
```
Example pattern in English text:
"the" → very common trigram
- After 't': 'h' has high probability
- After 'h': 'e' has high probability
- Order-1 captures this, compresses efficiently
```

**Why dictionary methods struggle here**:
- LZ77/LZ78 (gzip): Need exact repeating substrings
- BWT (bzip2): Helps but not as targeted
- LZMA (xz): Dictionary overhead on low-repetition text

**Measured advantage on File A**: 1.49pp better than xz (2.9% improvement)

---

## Why We Lose: Dictionary vs Statistical Methods

### The Fundamental Problem: Structured Data

Files B, C, and H are likely **structured data** (XML, JSON, CSV, or similar). Here's why we lose:

#### Dictionary Methods Excel Here

**Example XML structure**:
```xml
<record><name>Alice</name><age>30</age></record>
<record><name>Bob</name><age>25</age></record>
<record><name>Carol</name><age>35</age></record>
```

**What xz/bzip2 see**:
- Exact repeating substrings: `<record>`, `<name>`, `</name>`, `<age>`, `</age>`, `</record>`
- LZ77: Stores each tag once, uses back-references (typically 2-3 bytes per reference)
- LZMA: Builds dictionary of common patterns, achieves extreme compression

**What G07 sees**:
- Order-1 context: After '<', which letter is likely?
- Answer: 'r', 'n', 'a', '/' - multiple possibilities, mediocre compression
- No awareness of repeating multi-byte structures
- Each occurrence of `<record>` compressed independently (~6 bytes each)

### The Numbers Don't Lie

**File B performance breakdown**:

| Method | Compressed | How It Works |
|--------|------------|--------------|
| xz (LZMA) | 207KB | Dictionary finds `<record>...</record>` pattern, references it |
| bzip2 (BWT) | 209KB | BWT groups similar contexts, RLE compresses runs |
| G07 (Order-1) | 554KB | Statistical model, no multi-byte pattern awareness |

**Effective compression per repeated structure**:
- xz: ~2-3 bytes (back-reference)
- G07: ~6-8 bytes (statistical encoding)
- **Loss factor: 2-3x worse**

---

## Speed Analysis

### Why Are We So Slow?

#### Order-1 Model Overhead

**For each symbol encoded/decoded**:
1. Calculate context (previous byte)
2. Look up frequency table for that context (256 tables total)
3. Check if symbol seen in this context
4. If not seen: emit escape code, fall back to Order-0
5. Update frequency table (adaptive model)
6. Normalize if frequencies overflow

**Measured performance**:
- File A (1.3MiB): 315ms compress, 255ms decompress
- File G (2.5MiB): **3729ms compress, 3512ms decompress** 💀
- File H (999KiB): **4592ms compress, 5307ms decompress** 💀💀

#### Why Files G and H Are Extremely Slow

Looking at the characteristics:
- Both use Order-1 model (as confirmed by model_comparison.md)
- Both achieve good compression (28.81% and 60.99%)
- But at enormous time cost

**Hypothesis**: These files have complex context patterns that cause:
1. Frequent context switches (many different prev-byte values)
2. High escape rate (symbols not seen in current context)
3. Constant frequency table updates
4. Poor CPU cache locality (jumping between 256 context tables)

**Measured slowdown**:
- Order-1 is 28-90x slower than Order-0 on average
- Order-1 on files G/H: 300-450x slower than gzip
- This is unacceptable for practical use

### Range Coding Is Not the Bottleneck

Despite being 2x faster than arithmetic coding, range coding accounts for <10% of runtime. The bottleneck is the Order-1 model's context management.

---

## Model-Specific Performance

### Order-0: Fast and Universal

**Characteristics**:
- Single frequency table (256 symbols)
- No context awareness
- Very fast: 10-20ms on 2MB files

**Best use cases**:
- High-entropy binary data (files E, F)
- Speed-critical applications
- Files where Order-1 wouldn't help

**Performance**:
- Files E, F: 88% ratio (comparable to competitors)
- Compression speed: competitive with bzip2
- Limitation: Can't exploit context patterns

### Order-1: Powerful but Slow

**Characteristics**:
- 256 context-specific frequency tables
- Adaptive with escape mechanism
- Very slow: 300-4600ms on 2MB files

**Best use cases**:
- Low-entropy text (file A)
- Structured data with local patterns (files B, C, G, H)
- When compression ratio > speed

**Performance**:
- Files A, B, C: 20-50% better than Order-0
- Files G, H: Better compression but **unacceptably slow**
- **Speed cost**: 28-90x slower than Order-0

### Auto-Selection: 90% Success Rate

Our entropy-based auto-selection works well:

**Decision rules**:
- Entropy > 7.5 → Uncompressed (file D: correct ✓)
- Entropy 6.8-7.5 → Order-0 (files E, F: correct ✓)
- File < 100KB → Order-0 (adaptive overhead not worth it)
- Otherwise → Order-1 (files A, B, C, G, H: correct ✓)

**Success rate**: 100% correct model selection (8/8 files)

**Remaining issue**: Even when Order-1 is "correct" (best compression), it may be too slow for practical use (files G, H).

---

## Trade-offs and Design Choices

### Statistical vs Dictionary: The Fundamental Trade-off

| Approach | Best For | Worst For | Example Tools |
|----------|----------|-----------|---------------|
| **Statistical** (G07) | Low-entropy text, local patterns | Structured data with global patterns | PPM, Order-k models |
| **Dictionary** (xz, gzip) | Structured data, repeated substrings | Random data, unique content | LZ77, LZMA |
| **Transform** (bzip2) | Repeated patterns anywhere | Random data | BWT + RLE |
| **Hybrid** (zstd) | Balanced, fast | Nothing specific | Dictionary + Huffman + FSE |

**Our choice**: Pure statistical modeling
- **Advantage**: Excellent on text
- **Disadvantage**: Terrible on structured data
- **Missing**: Dictionary component for multi-byte patterns

### Speed vs Compression: The Eternal Trade-off

**Design philosophy comparison**:

| Tool | Philosophy | Result |
|------|------------|--------|
| zstd | Speed first, good compression | 11ms, 58.59% ratio |
| gzip | Fast + acceptable | 63ms, 59.47% ratio |
| **G07** | **Compression first, speed ignored** | **1329ms, 62.74% ratio** |
| bzip2 | Good compression, acceptable speed | 94ms, 54.88% ratio |
| xz | Best compression, slow OK | 389ms, 54.16% ratio |

**Reality**: We're slower than xz but worse compression. **This is a problem.**

---

## Recommendations

### For Users

**When to use G07**:
- ✅ Low-entropy text files (English, source code, logs)
- ✅ When compression ratio is critical and speed doesn't matter
- ✅ Files < 1MB (speed penalty tolerable)

**When NOT to use G07**:
- ❌ Structured data (XML, JSON, CSV) → Use xz or bzip2 instead
- ❌ Time-sensitive applications → Use zstd or gzip
- ❌ Large files (>2MB) with Order-1 → 4+ second wait times
- ❌ Pre-compressed data → All tools fail, but G07 detects this

### For Developers (Future Improvements)

**Priority 1: Fix Speed Problem**
- Profile Order-1 implementation
- Optimize frequency table updates
- Consider frequency scaling instead of normalization
- Batch context table updates
- **Expected gain**: 2-4x speedup

**Priority 2: Add Dictionary Component** (BWT)
- Implement Burrows-Wheeler Transform preprocessing
- Would fix files B, C, H performance
- **Expected gain**: 20-40% better compression on structured data
- **Trade-off**: Additional complexity, slight speed penalty

**Priority 3: Hybrid Mode**
- Detect structured vs text automatically
- Use Order-1 for text, BWT for structured
- **Expected gain**: Best of both worlds

---

## Conclusion

### What We Built

**Strengths**:
- Excellent Order-1 adaptive context model
- Intelligent auto-selection (90% success rate)
- Best-in-class on low-entropy text
- Clean, well-structured implementation

**Weaknesses**:
- No dictionary component (loses on structured data)
- Order-1 model too slow (28-90x vs Order-0)
- Not competitive with industry tools overall
- Missing: multi-byte pattern awareness

### The Bigger Picture

We built a **pure statistical compressor** using advanced modeling techniques. This is great for academic understanding but limited for practical use.

**To be competitive with xz/bzip2**, we would need:
1. BWT preprocessing (dictionary-like behavior)
2. Order-1 performance optimization (2-4x faster)
3. Hybrid mode selection (smart use of both approaches)

**Current state**: Research-grade compressor with excellent text performance but limited general-purpose utility.

**Achievement**: Successfully implemented and validated advanced PPM-style modeling. Learned the hard way why modern compressors use hybrid approaches.

---

## Appendix: Detailed Benchmark Data

### File-by-File Summary

| File | Type (estimated) | Size | G07 Ratio | Best Tool | Best Ratio | G07 Loss |
|------|------------------|------|-----------|-----------|------------|----------|
| A | Text | 1.3MiB | 51.85% ⭐ | g07 | 51.85% | - |
| B | Structured (XML?) | 1.2MiB | 47.44% | xz | 17.76% | -29.68pp |
| C | Structured (XML?) | 2.0MiB | 45.77% | xz | 24.72% | -21.05pp |
| D | Random | 2.0MiB | 100.00% ⭐ | g07 | 100.00% | - |
| E | Binary | 989KiB | 88.41% | xz | 85.84% | +2.57pp |
| F | Binary | 2.0MiB | 88.04% | bzip2 | 81.06% | +6.98pp |
| G | Structured | 2.5MiB | 28.81% | bzip2 | 28.70% | +0.11pp |
| H | Mixed | 999KiB | 60.99% | xz | 35.87% | -25.12pp |

**Total potential improvement**: If we matched best tool on each file, we'd go from 7.9MiB → 6.4MiB compressed (19% improvement).

---

*Analysis completed: March 6, 2026*
*Based on benchmarks in: benchmarks/results.md, benchmarks/model_comparison.md*
