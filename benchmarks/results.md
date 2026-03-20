# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-03-20
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
| B | 1.2MiB | **17.35%** ⭐ | 22.89% | 17.92% | 17.76% | 23.14% | g07 |
| C | 2.0MiB | **25.93%** ⭐ | 32.57% | 26.51% | **24.72%** ⭐ | 31.22% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.02% | 100.45% | 100.01% | 100.00% | g07 |
| E | 989KiB | **86.03%** ⭐ | 88.66% | 88.43% | **85.84%** ⭐ | 88.48% | xz |
| F | 2.0MiB | **82.42%** ⭐ | 88.01% | **81.06%** ⭐ | 82.38% | 87.78% | bzip2 |
| G | 2.5MiB | **29.28%** ⭐ | 36.99% | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | **44.72%** ⭐ | **44.02%** ⭐ | **42.34%** ⭐ | **35.87%** ⭐ | 44.96% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 168 ms | 108 | ✓ 597KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 97 ms | 26 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 163 ms | 105 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698040) | 🟡 53.34% | 4.2669 | 506 ms | 67 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700412) | 🟡 53.52% | 4.2814 | 27 ms | 10 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 83 ms | 63 | ✓ 944KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 63 ms | 16 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 121 ms | 51 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207576) | 🟢 17.76% | 1.4208 | 404 ms | 24 | ✓ 939KiB saved | ✓ |
| zstd | 265KiB (270395) | 🟢 23.14% | 1.8508 | 18 ms | 10 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 165 ms | 109 | ✓ 1.5MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 131 ms | 38 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 181 ms | 95 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494416) | 🟢 24.72% | 1.9777 | 700 ms | 51 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (624404) | 🟢 31.22% | 2.4976 | 27 ms | 11 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 23 ms | 11 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 86 ms | 19 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 277 ms | 181 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000160) | 🔴 100.01% | 8.0006 | 503 ms | 9 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 14 ms | 9 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 89 ms | 269 | ✓ 139KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 70 ms | 16 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 140 ms | 90 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868756) | 🔴 85.84% | 6.8669 | 239 ms | 89 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895471) | 🔴 88.48% | 7.0780 | 12 ms | 9 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 78 ms | 162 | ✓ 360KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 118 ms | 29 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 243 ms | 166 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727568) | 🔴 82.38% | 6.5901 | 558 ms | 173 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1840935) | 🔴 87.78% | 7.0226 | 23 ms | 15 | ✓ 251KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 723KiB (739764) | 🟢 29.28% | 2.3422 | 177 ms | 106 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 380 ms | 30 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 299 ms | 158 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750600) | 🟢 29.71% | 2.3765 | 1412 ms | 66 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905085) | 🟢 35.82% | 2.8656 | 34 ms | 15 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 447KiB (457416) | 🟢 44.72% | 3.5779 | 161 ms | 165 | ✓ 553KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 87 ms | 17 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 103 ms | 69 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366908) | 🟢 35.87% | 2.8699 | 358 ms | 39 | ✓ 641KiB saved | ✓ |
| zstd | 450KiB (459851) | 🟢 44.96% | 3.5969 | 19 ms | 9 | ✓ 550KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| zstd | 21 ms | 🥇 #1 |
| g07 | 118 ms | 🥈 #2 |
| gzip | 129 ms | 🥉 #3 |
| bzip2 | 190 ms |  #4 |
| xz | 585 ms |  #5 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 11 ms | 🥇 #1 |
| gzip | 23 ms | 🥈 #2 |
| xz | 64 ms | 🥉 #3 |
| bzip2 | 114 ms |  #4 |
| g07 | 124 ms |  #5 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 3 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **g07** | 54.93% | 3 | 6.9MiB | ⭐ |
|  4 | **zstd** | 58.59% | 0 | 7.4MiB |  |
|  5 | **gzip** | 59.47% | 0 | 7.5MiB |  |

### Files Won by Each Tool

- **g07**: 3 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 3 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07 | 13MiB | 6.9MiB | 54.93% | 4.3942 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3324 | 8 |
| zstd | 13MiB | 7.4MiB | 58.59% | 4.6872 | 8 |

---

*Report generated on Fri Mar 20 10:05:03 AM WET 2026*
