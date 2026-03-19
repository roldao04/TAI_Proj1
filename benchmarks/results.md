# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-03-19
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

| File | Size | G07 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|------|-------|----|----|--------|
| A | 1.3MiB | **53.30%** ⭐ | 58.49% | 54.05% | 53.34% | 53.52% | g07 |
| B | 1.2MiB | **17.35%** ⭐ | 22.89% | 17.92% | 17.76% | 23.11% | g07 |
| C | 2.0MiB | **25.93%** ⭐ | 32.57% | 26.51% | **24.72%** ⭐ | 31.19% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.02% | 100.45% | 100.01% | 100.00% | g07 |
| E | 989KiB | **85.60%** ⭐ | 88.66% | 88.43% | 85.84% | 88.53% | g07 |
| F | 2.0MiB | **81.70%** ⭐ | 88.01% | **81.06%** ⭐ | 82.38% | 87.81% | bzip2 |
| G | 2.5MiB | **29.28%** ⭐ | 36.99% | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | **44.72%** ⭐ | **44.02%** ⭐ | **42.34%** ⭐ | **35.88%** ⭐ | 44.86% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 121 ms | 73 | ✓ 597KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 56 ms | 14 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 98 ms | 62 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698048) | 🟡 53.34% | 4.2669 | 295 ms | 37 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700409) | 🟡 53.52% | 4.2813 | 16 ms | 13 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 65 ms | 41 | ✓ 944KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 45 ms | 13 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 95 ms | 44 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207584) | 🟢 17.76% | 1.4209 | 264 ms | 15 | ✓ 939KiB saved | ✓ |
| zstd | 264KiB (270069) | 🟢 23.11% | 1.8486 | 11 ms | 6 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 115 ms | 74 | ✓ 1.5MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 74 ms | 15 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 120 ms | 61 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494424) | 🟢 24.72% | 1.9777 | 418 ms | 40 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (623877) | 🟢 31.19% | 2.4955 | 24 ms | 15 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 14 ms | 13 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 62 ms | 8 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 166 ms | 110 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000168) | 🔴 100.01% | 8.0007 | 274 ms | 10 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 11 ms | 11 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 847KiB (866338) | 🔴 85.60% | 6.8478 | 75 ms | 183 | ✓ 143KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 48 ms | 11 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 93 ms | 57 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868764) | 🔴 85.84% | 6.8669 | 155 ms | 41 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895975) | 🔴 88.53% | 7.0820 | 13 ms | 11 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1.7MiB (1713365) | 🔴 81.70% | 6.5360 | 93 ms | 221 | ✓ 375KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 74 ms | 18 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 156 ms | 115 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727576) | 🔴 82.38% | 6.5902 | 328 ms | 62 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1841432) | 🔴 87.81% | 7.0245 | 11 ms | 13 | ✓ 250KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 723KiB (739764) | 🟢 29.28% | 2.3422 | 98 ms | 54 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 230 ms | 17 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 167 ms | 86 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750608) | 🟢 29.71% | 2.3765 | 877 ms | 36 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905141) | 🟢 35.82% | 2.8658 | 27 ms | 17 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 447KiB (457416) | 🟢 44.72% | 3.5779 | 120 ms | 96 | ✓ 553KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 50 ms | 9 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 74 ms | 47 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366916) | 🟢 35.88% | 2.8700 | 200 ms | 21 | ✓ 641KiB saved | ✓ |
| zstd | 449KiB (458829) | 🟢 44.86% | 3.5889 | 19 ms | 12 | ✓ 551KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| zstd | 16 ms | 🥇 #1 |
| gzip | 79 ms | 🥈 #2 |
| g07 | 87 ms | 🥉 #3 |
| bzip2 | 121 ms |  #4 |
| xz | 351 ms |  #5 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 12 ms | 🥇 #1 |
| gzip | 13 ms | 🥈 #2 |
| xz | 32 ms | 🥉 #3 |
| bzip2 | 72 ms |  #4 |
| g07 | 94 ms |  #5 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 2 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **g07** | 54.78% | 4 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐ |
|  4 | **zstd** | 58.58% | 0 | 7.4MiB |  |
|  5 | **gzip** | 59.47% | 0 | 7.5MiB |  |

### Files Won by Each Tool

- **g07**: 4 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 2 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07 | 13MiB | 6.9MiB | 54.78% | 4.3823 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3325 | 8 |
| zstd | 13MiB | 7.4MiB | 58.58% | 4.6867 | 8 |

---

*Report generated on Thu Mar 19 04:12:37 PM UTC 2026*
