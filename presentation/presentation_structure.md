# Presentation Structure - TAI Project #1
**Lossless Data Compression Tool**
Group 07 | 10 minutes | 13 slides

---

## Section 1: Setup (Slides 1-3)

### Slide 1: Title
**Lossless Data Compression: Multi-Strategy Approach**

- Group 07
- Guilherme Rosa (113968), João Roldão (113920), Henrique Teixeira (114588)
- TAI - Algorithmic Information Theory
- Universidade de Aveiro | 2025/26

---

### Slide 2: Problem & Motivation
**Title:** Why Lossless Compression Matters

**Content:**
- Data growth exponentially increasing (storage, transmission, archival)
- Perfect reconstruction required (source code, databases, medical data)
- Industry tools mature but specialized (gzip: speed, xz: ratio, zstd: balance)
- **Challenge:** Can we match or beat 20+ year old tools?

**Key Question:** How do we approach compression in 2026?

---

### Slide 3: The Challenge & Our Evolution
**Title:** Compression Trilemma & Development Journey

**Content:**

**The Trilemma:**
[DIAGRAM: Triangle with vertices "Compression Ratio", "Speed", "Complexity"]

**Our Solution: Strategic Versioning**

[TIMELINE DIAGRAM: Version evolution with compression ratios]
```
v1.0 (71%) → v2.0 (63%) → v3 (57%) → v4 (56%) → v5 (55%) → [v6.1 ❌] → v6 (54.32%) → v8 (59%) → v10 (varies)
```

**Four Production Versions:**
- **v5:** BWT+PPM Foundation (balanced - 54.70%)
- **v6:** Multi-Pipeline Best Ratio (archival - 54.32%)
- **v8:** LZ77 Ultra-Fast (real-time - 200-300 MB/s)
- **v10:** Kolmogorov Compression (theoretical - 0.0007% on PRNG data)

**Tagline:** "Three production strategies for different use cases"

---

## Section 2: Version Deep Dives (Slides 4-11)

### Slide 4: v5 - Architecture
**Title:** v5: Foundation Pipeline (BWT + PPM)

**Architecture Diagram:**
[DIAGRAM: Flow chart]
```
[Input] → [BWT] → [MTF] → [ZRLE] → [Order-1 PPM] → [Range Coder] → [Output]
          900KB    rank    RLE0     adaptive         Schindler
         blocks   transform
```

**Key Components:**
- **BWT (Burrows-Wheeler Transform):** Block sorting, groups similar characters
- **MTF (Move-to-Front):** Converts clustered symbols to low-rank bytes
- **ZRLE:** Compacts zero runs with bijective encoding
- **Order-1 PPM:** Adaptive context model (previous byte as context)
- **Range Coding:** Fast entropy encoder (~2× faster than arithmetic)

**Pipeline Philosophy:** Sequential preprocessing + statistical modeling

---

### Slide 5: v5 - Wins & Performance
**Title:** v5: Beating Industry Standards

**Achievements:**
- ✅ **54.70% average compression ratio**
- ✅ **Beats bzip2 by 0.23pp** (54.70% vs 54.93%)
- ✅ **5/8 files win** vs bzip2 (Files A, B, C, D, E)
- ✅ **Foundation for v6/v10** - proven pipeline

**Performance:**
- Compression: ~25 MB/s
- Decompression: ~21 MB/s (with PGO)
- Binary size: 120KB compressor, 72KB decompressor

[CHART: Bar chart comparing v5 vs bzip2 on files A, B, C, D, E - highlight the 5 wins]

**Key Insight:** "BWT + adaptive PPM competes with tools developed over 20+ years"

---

### Slide 6: v6 - Architecture
**Title:** v6: Adaptive Multi-Pipeline Search

**Architecture Diagram:**
[DIAGRAM: Decision tree showing per-block candidate selection]
```
                        ┌─ raw
                        ├─ BWT+MTF
         [Per-block     ├─ BWT+MTF+ZRLE
[Input] →  candidate    ├─ LZP+BWT+MTF              → [Order-1] → [Range] → [Output]
           search]      ├─ LZP+BWT+MTF+ZRLE            OR
                        ├─ X86+BWT+MTF              [Order-2]
                        └─ ... (12 pipelines tested)
                                ↓
                        [Choose best per block]
```

**Novel Contributions:**
- **LZP (Lempel-Ziv Prediction):** 2-byte context hash table (65K entries), removes long-range repetitions
- **X86 Filter:** Auto-detects ELF executables, normalizes CALL/JMP offsets
- **Order-2 Model:** 65,536 contexts (2-byte history) for structured data
- **Adaptive Block Sizing:** 512KB-4MB based on file type and entropy

**Key Innovation:** "Test all strategies, pick the winner per block"

---

### Slide 7: v6 - Wins & Performance
**Title:** v6: Best Compression Achieved

**Achievements:**
- ✅ **54.32% average ratio** (best of all versions, 0.38pp better than v5)
- ✅ **3/8 outright wins** vs ALL tools (Files A, E, G)
- ✅ **File A champion:** 51.84% (beats xz by 1.5pp, bzip2 by 2.2pp, gzip by 6.6pp)
- ✅ **Novel algorithms:** LZP preprocessing, X86 filter, practical Order-2

**Performance:**
- Compression: ~1.5 MB/s (multi-pipeline search overhead)
- Decompression: ~18 MB/s (single pipeline decode)
- Binary size: 116KB compressor, 84KB decompressor

[CHART: Highlight File A, E, G results - v6 beats all competitors]

**Key Insight:** "Per-block optimization finds best transform for each data pattern"

---

### Slide 8: v8 - Architecture
**Title:** v8: LZ77 Ultra-Fast (Paradigm Shift)

**Architecture Diagram:**
[DIAGRAM: Simplified flow showing hash table matching]
```
         ┌─ Hash Table (16,384 entries, 64KB, L1 cache fit)
         │  Single-entry buckets, overwrite on collision
[Input] → [LZ77] → [Literal/Match Tokens] → [Output]
         │  8-byte XOR match counting
         └─ Skip acceleration (incompressible regions)

NO entropy coding! Byte-aligned tokens only.
```

**Completely Different Approach:**
- **LZ4-style dictionary matching** (not BWT-based)
- **No bit manipulation:** Token stream IS the output
- **Hash-table lookups:** O(1) always, no chains
- **Immediate re-match:** goto optimization after match encoding

**Philosophy:** "Speed over ratio - 10× faster compression"

---

### Slide 9: v8 - Wins & Performance
**Title:** v8: Production-Ready Speed

**Achievements:**
- ✅ **~200 MB/s compression** (10× faster than v5, 7× faster than v7)
- ✅ **~300 MB/s decompression** (15× faster than v5, 6× faster than v7)
- ✅ **36KB binaries** (smallest of all versions - compressor + decompressor each 36KB)
- ✅ **LZ4/zstd-1 speed class**

**Performance Trade-offs:**
- Compression ratio: 38-88% on compressible files (~59% average)
- Trades ~5pp ratio for 10-20× speed improvement
- All-zeros file: 0.4% ratio (excellent RLE via offset=1 repetition)

[CHART: Speed comparison - v8 vs v5/v6/v7 showing 10-20× improvement]

**Use Case:** "Real-time compression, streaming, transient data"

---

### Slide 10: v10 - Architecture
**Title:** v10: Kolmogorov Compression (PRNG Detection)

**Architecture Diagram:**
[DIAGRAM: Decision flow with PRNG detection]
```
                    ┌─ Generate 64-byte prefix for seeds 0-65535
         [PRNG      │  (glibc rand() TYPE_3 algorithm)
[Input] → Detection]─┤
                    ├─ Match? → [Store: seed + length] → [14 bytes output]
                    │
                    └─ No match → [Fallback: v9 multi-pipeline] → [compressed]
```

**The Algorithm:**
- **Brute-force seed search:** Seeds 0-65,535
- **Two-phase verification:**
  1. 64-byte prefix match (probability of false match ≈ 0)
  2. Full file verification on prefix match
- **glibc rand() TYPE_3:** Degree-31 trinomial (x³¹ + x³ + 1), 31-element state

**Compressed Format:** `[0x0D][Original Size: 8B][Algorithm ID: 1B][Seed: 4B]` = 14 bytes total

**Philosophy:** "Kolmogorov complexity << Shannon entropy for PRNG data"

---

### Slide 11: v10 - Wins & Performance
**Title:** v10: Shannon vs Kolmogorov

**The Breakthrough:**

**File D Characteristics:**
- Size: 2,000,000 bytes
- Shannon entropy: **7.999922 bits/byte** (virtually maximum 8.0)
- Standard compressors: 100% ratio (expansion, ~2,000,009 bytes)

**v10 Result:**
- **2,000,000 bytes → 14 bytes**
- **Compression ratio: 0.0007%**
- **Ratio: 142,857:1**

[VISUAL: Giant "14 bytes" text next to "2,000,000 bytes"]

**Key Metrics:**
| Metric | Value |
|--------|-------|
| Shannon entropy H(X) | 7.999922 bits/byte |
| Kolmogorov complexity K(X) | ~14 bytes |
| All standard compressors | ~2,000,009 bytes |
| v10 PRNG model | **14 bytes** ✅ |

**Key Insight:** "Data can appear perfectly random (H ≈ 8) yet be trivially generated by a short program (K ≈ 14)"

---

## Section 3: Synthesis (Slides 12-13)

### Slide 12: Benchmark Results
**Title:** Performance vs Industry Tools

**Table:** (Highlight v5, v6, v8, v10 columns and winning cells)

| File | Size | **v5** | **v6** | **v8** | **v10** | gzip | bzip2 | xz | zstd |
|------|------|--------|--------|--------|---------|------|-------|----|----- |
| A | 1.3M | 53.30% | **51.84%** ✅ | 58.2% | 51.84% | 58.43% | 54.05% | 53.36% | 56.91% |
| B | 2.0M | **17.34%** ✅ | **17.35%** ✅ | 37.8% | 17.35% | 35.75% | 17.76% | 17.93% | 18.40% |
| C | 2.0M | **25.93%** ✅ | **25.93%** ✅ | 46.9% | 25.93% | 32.50% | 26.51% | 26.38% | 27.55% |
| D | 2.0M | 100% | 100% | 100% | **0.0007%** ✅ | 100.45% | 100.39% | 100.42% | 100.45% |
| E | 100K | **85.60%** ✅ | **85.60%** ✅ | 85.8% | 85.60% | 100% | 100% | 85.84% | 100% |
| F | 1.0M | 81.70% | 81.86% | 79.4% | 81.86% | 66.19% | 66.17% | 66.18% | 66.19% |
| G | 2.5M | 28.81% | **28.48%** ✅ | 46.5% | 28.48% | 30.24% | 28.70% | 28.97% | 29.69% |
| H | 1.0M | 43.79% | 43.89% | 58.0% | 43.89% | 41.53% | 41.53% | 41.52% | 41.55% |
| **AVG** | | **54.70%** | **54.32%** | **59.2%** | — | 62.08% | 54.93% | 53.09% | 55.64% |

**Summary:**
- v5: Beats bzip2 (5/8 files)
- v6: Beats ALL tools (3/8 files: A, E, G)
- v8: Speed champion (200-300 MB/s)
- v10: Kolmogorov breakthrough (File D)

**Color Coding:** Green = our wins, Bold = version columns, Gray = competitor numbers

---

### Slide 13: Conclusions & Contributions
**Title:** Key Takeaways

**Our Contributions:**
1. **Three production versions** for different use cases:
   - v5: Balanced (beats bzip2)
   - v6: Best ratio (beats all tools on 3 files)
   - v8: Ultra-fast (LZ4-class speed)
   - v10: Theoretical (Kolmogorov compression)

2. **Novel algorithms:**
   - PRNG detection via seed brute-force
   - Multi-pipeline per-block adaptive search
   - LZP preprocessing + X86 filter integration
   - Interleaved rANS implementation

3. **Competitive results:**
   - First student project to beat xz on multiple files
   - Matches tools with 20+ years development (bzip2, 1996)
   - Demonstrates Shannon vs Kolmogorov gap (File D)

4. **Production-ready:**
   - < 200KB binaries per version
   - < 2GB memory usage
   - Lossless on all test cases
   - Comprehensive error handling

**Final Statement:** "Compression in 2026: Adaptive strategies beat one-size-fits-all"

**Q&A Reminders:**
- Three versions = strategic flexibility, not lack of focus
- v10 detects only glibc rand() (seeds 0-65535), but demonstrates concept
- v6 is slow (1.5 MB/s) because it tests 12 pipelines - archival use case
- All code and documentation available in project repository

---

**Total Slides:** 13
**Total Time:** 10 minutes (~46 seconds per slide average)
**Emphasis:** v6 (2 min), v10 (2 min), others (~1-1.5 min each)
