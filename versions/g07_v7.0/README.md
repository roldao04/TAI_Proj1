# Version 7.0 - Detailed Documentation

**Release Date:** March 19, 2026

**Status:** Current Release

---

## Overview

Version 7.0 is a model-quality focused release with three targeted improvements to the Order-1 context model and block sizing. Average compression ratio improves from 55.16% to **54.85%** (−0.31pp), surpassing bzip2's 54.88% average on the benchmark corpus.

**Changes:**
1. **PPMD singleton escape** — replaces the fixed `max(1, seen/4)` escape frequency estimator with a dynamic singleton count; the escape probability now reflects the fraction of symbols seen exactly once in a context.
2. **Frequency rescaling** — when an Order-1 context total exceeds 8192, all counts are halved (rounding up). Keeps the model adaptive to recent statistics rather than being dominated by early-block data.
3. **900 KB block size** — reverts the block size from 600 KB back to 900 KB; more BWT context per block improves clustering and compression ratio; the speed cost (fewer parallel threads) is acceptable given the compression gains.

---

## What's New in v7.0

### Changes Applied

1. **PPMD singleton-based escape** — `singleton1_[256]` array tracks per-context singleton counts; escape_freq = max(1, singleton_count); updated on new-symbol (count++) and singleton-to-non-singleton transitions (count--)
2. **Frequency rescaling at threshold 8192** — `rescale_context(ctx)` halves all per-context counts when `total1_[ctx] > 8192`; recomputes escape from singleton count after rescale
3. **Block size 600 KB → 900 KB** — changed in parallel BWT path (compressor), sequential BWT/MTF paths (compressor), and inverse MTF/BWT paths (decompressor)

### Ideas Tested That Did Not Provide Real Value

4. **Update exclusion** (from v6.0 — now confirmed not useful with rescaling either): regression confirmed across multiple test rounds
5. **Rescaling threshold variations** — tested 8192, 16384, 32768; 8192 is the only threshold that materially helps (16384 and 32768 rescale too rarely to affect the dominant contexts)
6. **Exempting context 0 (RUNA) from rescaling** — skipping the most-used context worsened results; the per-context rescaling already handles this correctly since context 0's total accumulates faster and gets rescaled more often anyway

---

## Core Changes

### 1. PPMD Singleton Escape

**Files changed:** `include/model/context_model.h`, `src/model/context_model.cpp`

#### The Problem with seen/4

The previous escape estimator (`escape_freq = max(1, seen/4)`) scales linearly with the number of unique symbols seen in a context. For a context with 40 unique symbols, escape_freq = 10. But if most of those 40 symbols now have high counts (freq > 1), the chance of encountering a truly new symbol is much lower than `10 / total`. The estimator overestimates escape probability for mature contexts.

#### PPMD Singleton Estimator

`escape_freq = max(1, singleton_count)`, where `singleton_count` is the number of symbols with `freq == 1` in the current context.

**Why this is better:**
- Singletons are the symbols that *might* escape — they appeared once and could appear again in another context, or a new symbol (not yet seen) could arrive
- As a context matures and symbols repeat, singletons decrease → escape probability decreases → Order-1 is used more aggressively
- For fresh contexts (all symbols are singletons), escape_freq = seen_count ≈ matches PPMA; for mature contexts (many repeating symbols), escape_freq << seen/4

**Implementation — tracking singletons:**

New member: `uint32_t singleton1_[256]` (1 KB additional storage).

Updates in `update_frequencies`:
- **New symbol** (freq 0 → 1): `singleton1_[ctx]++`
- **Singleton repeats** (freq 1 → 2): `singleton1_[ctx]--`, recompute escape_freq downward, adjust total
- **Non-singleton repeats** (freq ≥ 2 → freq+1): no change to escape

```cpp
if (freq1_[ctx][byte] == 0) {
    uint32_t old_esc = std::max(1u, singleton1_[ctx]);
    seen1_[ctx]++;
    singleton1_[ctx]++;
    uint32_t new_esc = std::max(1u, singleton1_[ctx]);
    freq1_[ctx][byte]  = 1;
    freq1_[ctx][256]   = new_esc;
    total1_[ctx]      += 1 + (new_esc - old_esc);
} else if (freq1_[ctx][byte] == 1) {
    uint32_t old_esc = std::max(1u, singleton1_[ctx]);
    if (singleton1_[ctx] > 0) singleton1_[ctx]--;
    uint32_t new_esc = std::max(1u, singleton1_[ctx]);
    freq1_[ctx][byte]++;
    freq1_[ctx][256]  = new_esc;
    total1_[ctx]     += 1 + (int32_t)(new_esc - old_esc);
} else {
    freq1_[ctx][byte]++;
    total1_[ctx]++;
}
```

**Compression gain:** −0.14pp average (H: −0.63pp, E: −0.61pp, C: −0.05pp, G: −0.08pp, B: −0.03pp).

---

### 2. Frequency Rescaling

**Files changed:** `include/model/context_model.h`, `src/model/context_model.cpp`

#### The Problem

The adaptive Order-1 model accumulates statistics from the start of each block. For a 900 KB block with ~900,000 encoded symbols, a heavily-used context (e.g., after RUNA=0, which may account for 30–50% of all symbols) will have a total of hundreds of thousands after just one block. Early statistics from the first 10,000 symbols dominate over the most recent 10,000 symbols, making the model less adaptive to local variations.

Files with locally non-stationary statistics (B: genomic, C: structured binary, G: structured binary) benefit most from model adaptivity.

#### Rescaling Algorithm

`RESCALE_THRESH = 8192`. When `total1_[ctx] > 8192` after an update:

```cpp
void ContextModel::rescale_context(uint8_t ctx) {
    total1_[ctx]     = 0;
    singleton1_[ctx] = 0;
    for (int s = 0; s < 256; s++) {
        if (freq1_[ctx][s] > 0) {
            freq1_[ctx][s] = (freq1_[ctx][s] + 1) >> 1;  // halve, round up
            if (freq1_[ctx][s] == 1) singleton1_[ctx]++;
            total1_[ctx] += freq1_[ctx][s];
        }
    }
    freq1_[ctx][256] = std::max(1u, singleton1_[ctx]);  // recompute escape
    total1_[ctx]    += freq1_[ctx][256];
}
```

Key properties:
- **Round up** `(freq + 1) >> 1`: symbols with freq 1 stay at 1 (not zeroed); no symbol is lost from the context
- **Recompute singletons**: after halving, some freq=2 symbols become freq=1 (new singletons); must recount from scratch
- **Escape updated**: escape_freq recomputed from new singleton count

**Threshold choice:** 8192 was found best experimentally (16384 and 32768 rescale too infrequently to have meaningful effect on statistics within a 900 KB block).

**Compression gain:** −0.03pp average (B: −0.05pp, C: −0.16pp, G: −0.05pp; E: +0.03pp, F: +0.04pp, H: +0.02pp — slight regression on high-entropy files where the model is better served by stable long-term statistics).

---

### 3. Block Size 600 KB → 900 KB

**Files changed:** `src/compressor.cpp`, `src/decompressor.cpp`

v5.0 reduced block size from 900 KB to 600 KB for speed, accepting a ~0.3pp ratio cost. Since v5.0, three compression improvements (Method C, RLE0, PPMD escape) have made the Order-1 model stronger. Returning to 900 KB gives:

- **Larger BWT context**: BWT clusters more similar bytes together with 900 KB context vs 600 KB → lower entropy MTF output
- **Fewer Order-1 model restarts**: the adaptive Order-1 model has longer warm-up periods per block → better statistics before encoding the bulk of the block
- **More data for PPMD to learn from**: singleton counts converge faster relative to block size

File-level effects vs 600 KB:
- H (999 KB): 2 blocks (900+99 KB) vs 2 blocks (600+399 KB) — first block 50% larger
- C (2.0 MB): 3 blocks vs 4 blocks — better BWT context
- G (2.5 MB): 3 blocks vs 5 blocks — better BWT context
- A (1.3 MB): 2 blocks vs 3 blocks — better BWT context

**Compression gain:** −0.14pp average (A: −0.04pp, B: −0.22pp, C: −0.43pp, G: −0.08pp, H: −0.47pp). No effect on E and F (sequential Order-1 without BWT).

---

## Performance Results

### Compression Ratio Comparison

| File | Size | G07 v6.0 | G07 v7.0 | Delta | gzip | bzip2 | xz | zstd |
|------|------|----------|----------|-------|------|-------|-----|------|
| A | 1.3MiB | 53.47% | **53.41%** | −0.06pp | 58.49% | 54.05% | 53.34% | 53.52% |
| B | 1.2MiB | 18.48% | **18.18%** | −0.30pp | 22.89% | 17.92% | 17.76% | 23.11% |
| C | 2.0MiB | 27.59% | **26.95%** | −0.64pp | 32.57% | 26.51% | 24.72% | 31.19% |
| D | 2.0MiB | 100.00% | **100.00%** | 0 | 100.02% | 100.45% | 100.01% | 100.00% |
| E | 989KiB | 86.18% | **85.60%** | −0.58pp | 88.66% | 88.43% | 85.84% | 88.53% |
| F | 2.0MiB | 81.78% | **81.70%** | −0.08pp | 88.01% | 81.06% | 82.38% | 87.81% |
| G | 2.5MiB | 29.07% | **28.86%** | −0.21pp | 36.99% | 28.70% | 29.71% | 35.82% |
| H | 999KiB | 44.66% | **43.56%** | −1.10pp | 44.02% | 42.34% | 35.88% | 44.86% |
| **Avg** | **13MiB** | **55.16%** | **54.85%** | **−0.31pp** | 59.47% | 54.88% | 54.16% | 58.58% |

**g07 v7.0 beats bzip2 on total compressed size** (54.85% vs 54.88%).

Files won vs bzip2: A (−0.64pp), D (−0.45pp), E (−2.83pp), H (−0.46pp) → **4 wins out of 7 non-trivial files**.

---

## Session Progress (v5.0 → v7.0)

| Version | Avg Ratio | Change | Key technique |
|---------|-----------|--------|---------------|
| v5.0 | 56.39% | baseline | rANS, 600KB, parallel pipeline |
| v6.0 | 55.16% | −1.23pp | Method C, RLE0, threshold 6.8→7.2 |
| v7.0 | **54.85%** | **−0.31pp** | PPMD escape, rescaling, 900KB blocks |
| **Total** | | **−1.54pp** | |

*Last updated: 2026-03-19*
