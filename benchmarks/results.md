# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-04-11
**Tools Compared**: G07, gzip, bzip2, xz (LZMA), zstd

---

## Table of Contents
1. [Quick Summary](#quick-summary)
2. [Detailed Results by File](#detailed-results-by-file)
3. [Performance Metrics](#performance-metrics)
4. [Rankings](#rankings)
5. [Overall Statistics](#overall-statistics)

---

## Quick Summary

### Compression Ratio Comparison

| File | Size | g07-v5 | g07-v8 | g07-v9 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|----------|----------|------|-------|----|----|--------|
| A | 1.3MiB | 53.30% | 87.97% | **51.84%** ⭐ | 58.51% | 54.05% | 53.34% | 53.52% | g07-v9 |
| B | 1.2MiB | **17.35%** ⭐ | 37.84% | 17.35% | 22.91% | 17.92% | 17.76% | 23.11% | g07-v5 |
| C | 2.0MiB | 25.93% | 45.22% | 25.93% | 32.39% | 26.51% | **24.72%** ⭐ | 31.19% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.00% | 100.00% | 100.03% | 100.45% | 100.01% | 100.00% | g07-v5 |
| E | 989KiB | 86.03% | 100.00% | **85.60%** ⭐ | 88.79% | 88.43% | 85.84% | 88.53% | g07-v9 |
| F | 2.0MiB | 82.42% | 100.00% | 81.70% | 88.17% | **81.06%** ⭐ | 82.38% | 87.81% | bzip2 |
| G | 2.5MiB | 28.82% | 72.76% | **28.48%** ⭐ | 37.02% | 28.70% | 29.71% | 35.82% | g07-v9 |
| H | 999KiB | 43.79% | 58.02% | 43.64% | 43.98% | 42.34% | **35.88%** ⭐ | 44.86% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 136 ms | 99 | ✓ 597KiB saved | ✓ |
| g07-v8 | 1.1MiB (1151299) | 🔴 87.97% | 7.0375 | 27 ms | 24 | ✓ 154KiB saved | ✓ |
| g07-v9 | 663KiB (678467) | 🟡 51.84% | 4.1472 | 422 ms | 108 | ✓ 616KiB saved | ✓ |
| gzip | 748KiB (765745) | 🟡 58.51% | 4.6807 | 87 ms | 24 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 138 ms | 77 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698048) | 🟡 53.34% | 4.2669 | 377 ms | 72 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700409) | 🟡 53.52% | 4.2813 | 36 ms | 23 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 77 ms | 61 | ✓ 944KiB saved | ✓ |
| g07-v8 | 432KiB (442203) | 🟢 37.84% | 3.0268 | 23 ms | 23 | ✓ 710KiB saved | ✓ |
| g07-v9 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 710 ms | 60 | ✓ 944KiB saved | ✓ |
| gzip | 262KiB (267716) | 🟢 22.91% | 1.8325 | 58 ms | 20 | ✓ 880KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 117 ms | 50 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207584) | 🟢 17.76% | 1.4209 | 333 ms | 37 | ✓ 939KiB saved | ✓ |
| zstd | 264KiB (270069) | 🟢 23.11% | 1.8486 | 27 ms | 21 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 129 ms | 100 | ✓ 1.5MiB saved | ✓ |
| g07-v8 | 884KiB (904314) | 🟢 45.22% | 3.6173 | 29 ms | 29 | ✓ 1.1MiB saved | ✓ |
| g07-v9 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 1028 ms | 102 | ✓ 1.5MiB saved | ✓ |
| gzip | 633KiB (647846) | 🟢 32.39% | 2.5914 | 94 ms | 24 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 169 ms | 82 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494424) | 🟢 24.72% | 1.9777 | 547 ms | 59 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (623877) | 🟢 31.19% | 2.4955 | 30 ms | 24 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 26 ms | 24 | ✗  overhead | ✓ |
| g07-v8 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 33 ms | 22 | ✗  overhead | ✓ |
| g07-v9 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 23 ms | 22 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000635) | 🔴 100.03% | 8.0025 | 76 ms | 19 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 277 ms | 143 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000168) | 🔴 100.01% | 8.0007 | 353 ms | 24 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 21 ms | 19 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 97 ms | 203 | ✓ 139KiB saved | ✓ |
| g07-v8 | 989KiB (1012121) | 🔴 100.00% | 8.0001 | 20 ms | 20 | ✗  overhead | ✓ |
| g07-v9 | 847KiB (866359) | 🔴 85.60% | 6.8479 | 1078 ms | 264 | ✓ 143KiB saved | ✓ |
| gzip | 878KiB (898683) | 🔴 88.79% | 7.1034 | 72 ms | 22 | ✓ 111KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 140 ms | 79 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868764) | 🔴 85.84% | 6.8669 | 189 ms | 75 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895975) | 🔴 88.53% | 7.0820 | 22 ms | 21 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 59 ms | 106 | ✓ 360KiB saved | ✓ |
| g07-v8 | 2.1MiB (2097161) | 🔴 100.00% | 8.0000 | 23 ms | 23 | ✗  overhead | ✓ |
| g07-v9 | 1.7MiB (1713386) | 🔴 81.70% | 6.5360 | 1665 ms | 303 | ✓ 375KiB saved | ✓ |
| gzip | 1.8MiB (1849027) | 🔴 88.17% | 7.0535 | 113 ms | 27 | ✓ 243KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 242 ms | 136 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727576) | 🔴 82.38% | 6.5902 | 439 ms | 139 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1841432) | 🔴 87.81% | 7.0245 | 25 ms | 23 | ✓ 250KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 712KiB (728184) | 🟢 28.82% | 2.3055 | 124 ms | 82 | ✓ 1.8MiB saved | ✓ |
| g07-v8 | 1.8MiB (1838351) | 🟡 72.76% | 5.8204 | 34 ms | 35 | ✓ 673KiB saved | ✓ |
| g07-v9 | 703KiB (719531) | 🟢 28.48% | 2.2781 | 489 ms | 67 | ✓ 1.8MiB saved | ✓ |
| gzip | 914KiB (935357) | 🟢 37.02% | 2.9615 | 315 ms | 26 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 231 ms | 99 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750608) | 🟢 29.71% | 2.3765 | 1142 ms | 71 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905141) | 🟢 35.82% | 2.8658 | 38 ms | 24 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 438KiB (447874) | 🟢 43.79% | 3.5033 | 137 ms | 137 | ✓ 562KiB saved | ✓ |
| g07-v8 | 580KiB (593367) | 🟡 58.02% | 4.6413 | 23 ms | 23 | ✓ 420KiB saved | ✓ |
| g07-v9 | 436KiB (446377) | 🟢 43.64% | 3.4915 | 903 ms | 86 | ✓ 563KiB saved | ✓ |
| gzip | 440KiB (449790) | 🟢 43.98% | 3.5182 | 80 ms | 21 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 102 ms | 61 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366916) | 🟢 35.88% | 2.8700 | 271 ms | 47 | ✓ 641KiB saved | ✓ |
| zstd | 449KiB (458829) | 🟢 44.86% | 3.5889 | 25 ms | 21 | ✓ 551KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| g07-v8 | 26 ms | 🥇 #1 |
| zstd | 28 ms | 🥈 #2 |
| g07-v5 | 98 ms | 🥉 #3 |
| gzip | 111 ms |  #4 |
| bzip2 | 177 ms |  #5 |
| xz | 456 ms |  #6 |
| g07-v9 | 789 ms |  #7 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| gzip | 22 ms | 🥇 #1 |
| zstd | 22 ms | 🥈 #2 |
| g07-v8 | 24 ms | 🥉 #3 |
| xz | 65 ms |  #4 |
| bzip2 | 90 ms |  #5 |
| g07-v5 | 101 ms |  #6 |
| g07-v9 | 126 ms |  #7 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 2 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **g07-v9** | 54.40% | 3 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **g07-v5** | 54.77% | 2 | 6.9MiB | ⭐ |
|  4 | **bzip2** | 54.88% | 1 | 6.9MiB |  |
|  5 | **zstd** | 58.58% | 0 | 7.4MiB |  |
|  6 | **gzip** | 59.49% | 0 | 7.5MiB |  |
|  7 | **g07-v8** | 76.42% | 0 | 9.6MiB |  |

### Files Won by Each Tool

- **g07-v5**: 2 file(s)
- **g07-v8**: 0 file(s)
- **g07-v9**: 3 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 1 file(s)
- **xz**: 2 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07-v5 | 13MiB | 6.9MiB | 54.77% | 4.3814 | 8 |
| g07-v8 | 13MiB | 9.6MiB | 76.42% | 6.1136 | 8 |
| g07-v9 | 13MiB | 6.9MiB | 54.40% | 4.3516 | 8 |
| gzip | 13MiB | 7.5MiB | 59.49% | 4.7592 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3325 | 8 |
| zstd | 13MiB | 7.4MiB | 58.58% | 4.6867 | 8 |

---

*Report generated on Sat Apr 11 18:17:08 WEST 2026*
