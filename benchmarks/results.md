# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-04-09
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

| File | Size | g07-v5 | g07-v7 | g07-v8 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|----------|----------|------|-------|----|----|--------|
| A | 1.3MiB | **53.30%** ⭐ | 53.39% | 87.97% | 58.49% | 54.05% | 53.34% | 53.52% | g07-v5 |
| B | 1.2MiB | **17.35%** ⭐ | 20.62% | 37.84% | 22.89% | 17.92% | 17.76% | 23.11% | g07-v5 |
| C | 2.0MiB | 25.93% | 31.94% | 45.22% | 32.57% | 26.51% | **24.72%** ⭐ | 31.19% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.00% | 100.00% | 100.02% | 100.45% | 100.01% | 100.00% | g07-v5 |
| E | 989KiB | 86.03% | 88.17% | 100.00% | 88.66% | 88.43% | **85.84%** ⭐ | 88.53% | xz |
| F | 2.0MiB | 82.42% | 87.62% | 100.00% | 88.01% | **81.06%** ⭐ | 82.38% | 87.81% | bzip2 |
| G | 2.5MiB | 28.82% | 39.24% | 72.76% | 36.99% | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | 43.79% | 46.93% | 58.02% | 44.02% | 42.34% | **35.88%** ⭐ | 44.86% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 112 ms | 76 | ✓ 597KiB saved | ✓ |
| g07-v7 | 683KiB (698690) | 🟡 53.39% | 4.2708 | 69 ms | 42 | ✓ 596KiB saved | ✓ |
| g07-v8 | 1.1MiB (1151299) | 🔴 87.97% | 7.0375 | 15 ms | 15 | ✓ 154KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 61 ms | 17 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 98 ms | 59 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698048) | 🟡 53.34% | 4.2669 | 291 ms | 40 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700409) | 🟡 53.52% | 4.2813 | 17 ms | 13 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 64 ms | 37 | ✓ 944KiB saved | ✓ |
| g07-v7 | 236KiB (240989) | 🟢 20.62% | 1.6495 | 56 ms | 35 | ✓ 907KiB saved | ✓ |
| g07-v8 | 432KiB (442203) | 🟢 37.84% | 3.0268 | 13 ms | 14 | ✓ 710KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 41 ms | 12 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 89 ms | 36 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207584) | 🟢 17.76% | 1.4209 | 268 ms | 21 | ✓ 939KiB saved | ✓ |
| zstd | 264KiB (270069) | 🟢 23.11% | 1.8486 | 17 ms | 11 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 109 ms | 69 | ✓ 1.5MiB saved | ✓ |
| g07-v7 | 624KiB (638869) | 🟢 31.94% | 2.5555 | 76 ms | 53 | ✓ 1.3MiB saved | ✓ |
| g07-v8 | 884KiB (904314) | 🟢 45.22% | 3.6173 | 17 ms | 16 | ✓ 1.1MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 81 ms | 19 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 125 ms | 60 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494424) | 🟢 24.72% | 1.9777 | 407 ms | 35 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (623877) | 🟢 31.19% | 2.4955 | 22 ms | 14 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 16 ms | 12 | ✗  overhead | ✓ |
| g07-v7 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 12 ms | 13 | ✗  overhead | ✓ |
| g07-v8 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 13 ms | 13 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 63 ms | 11 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 195 ms | 109 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000168) | 🔴 100.01% | 8.0007 | 268 ms | 12 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 14 ms | 11 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 79 ms | 141 | ✓ 139KiB saved | ✓ |
| g07-v7 | 872KiB (892377) | 🔴 88.17% | 7.0536 | 13 ms | 19 | ✓ 117KiB saved | ✓ |
| g07-v8 | 989KiB (1012121) | 🔴 100.00% | 8.0001 | 14 ms | 10 | ✗  overhead | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 63 ms | 13 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 103 ms | 59 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868764) | 🔴 85.84% | 6.8669 | 139 ms | 40 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895975) | 🔴 88.53% | 7.0820 | 15 ms | 12 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 52 ms | 83 | ✓ 360KiB saved | ✓ |
| g07-v7 | 1.8MiB (1837503) | 🔴 87.62% | 7.0095 | 20 ms | 28 | ✓ 254KiB saved | ✓ |
| g07-v8 | 2.1MiB (2097161) | 🔴 100.00% | 8.0000 | 13 ms | 12 | ✗  overhead | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 84 ms | 16 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 161 ms | 108 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727576) | 🔴 82.38% | 6.5902 | 302 ms | 69 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1841432) | 🔴 87.81% | 7.0245 | 13 ms | 13 | ✓ 250KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 712KiB (728184) | 🟢 28.82% | 2.3055 | 97 ms | 52 | ✓ 1.8MiB saved | ✓ |
| g07-v7 | 969KiB (991593) | 🟢 39.24% | 3.1395 | 125 ms | 60 | ✓ 1.5MiB saved | ✓ |
| g07-v8 | 1.8MiB (1838351) | 🟡 72.76% | 5.8204 | 17 ms | 17 | ✓ 673KiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 248 ms | 16 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 164 ms | 88 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750608) | 🟢 29.71% | 2.3765 | 808 ms | 38 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905141) | 🟢 35.82% | 2.8658 | 22 ms | 15 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 438KiB (447874) | 🟢 43.79% | 3.5033 | 110 ms | 100 | ✓ 562KiB saved | ✓ |
| g07-v7 | 469KiB (479971) | 🟢 46.93% | 3.7543 | 55 ms | 41 | ✓ 531KiB saved | ✓ |
| g07-v8 | 580KiB (593367) | 🟡 58.02% | 4.6413 | 17 ms | 13 | ✓ 420KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 61 ms | 13 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 77 ms | 44 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366916) | 🟢 35.88% | 2.8700 | 212 ms | 28 | ✓ 641KiB saved | ✓ |
| zstd | 449KiB (458829) | 🟢 44.86% | 3.5889 | 21 ms | 12 | ✓ 551KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| g07-v8 | 14 ms | 🥇 #1 |
| zstd | 17 ms | 🥈 #2 |
| g07-v7 | 53 ms | 🥉 #3 |
| g07-v5 | 79 ms |  #4 |
| gzip | 87 ms |  #5 |
| bzip2 | 126 ms |  #6 |
| xz | 336 ms |  #7 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 12 ms | 🥇 #1 |
| g07-v8 | 13 ms | 🥈 #2 |
| gzip | 14 ms | 🥉 #3 |
| xz | 35 ms |  #4 |
| g07-v7 | 36 ms |  #5 |
| bzip2 | 70 ms |  #6 |
| g07-v5 | 71 ms |  #7 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 3 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **g07-v5** | 54.77% | 3 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐ |
|  4 | **zstd** | 58.58% | 0 | 7.4MiB |  |
|  5 | **g07-v7** | 59.23% | 0 | 7.5MiB |  |
|  6 | **gzip** | 59.47% | 0 | 7.5MiB |  |
|  7 | **g07-v8** | 76.42% | 0 | 9.6MiB |  |

### Files Won by Each Tool

- **g07-v5**: 3 file(s)
- **g07-v7**: 0 file(s)
- **g07-v8**: 0 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 3 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07-v5 | 13MiB | 6.9MiB | 54.77% | 4.3814 | 8 |
| g07-v7 | 13MiB | 7.5MiB | 59.23% | 4.7380 | 8 |
| g07-v8 | 13MiB | 9.6MiB | 76.42% | 6.1136 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3325 | 8 |
| zstd | 13MiB | 7.4MiB | 58.58% | 4.6867 | 8 |

---

*Report generated on Thu Apr  9 12:13:36 PM UTC 2026*
