# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-03-06
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
| A | 1.3MiB | **51.85%** ⭐ | 58.49% | 54.05% | 53.34% | 53.52% | g07 |
| B | 1.2MiB | **47.44%** ⭐ | **22.89%** ⭐ | **17.92%** ⭐ | **17.76%** ⭐ | 23.14% | xz |
| C | 2.0MiB | **45.77%** ⭐ | **32.57%** ⭐ | **26.51%** ⭐ | **24.72%** ⭐ | 31.22% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.02% | 100.45% | 100.01% | 100.00% | g07 |
| E | 989KiB | **88.41%** ⭐ | 88.66% | 88.43% | **85.84%** ⭐ | 88.48% | xz |
| F | 2.0MiB | **88.04%** ⭐ | **88.01%** ⭐ | **81.06%** ⭐ | 82.38% | 87.78% | bzip2 |
| G | 2.5MiB | **28.81%** ⭐ | 36.99% | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | **60.99%** ⭐ | **44.02%** ⭐ | **42.34%** ⭐ | **35.87%** ⭐ | 44.96% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 663KiB (678619) | 🟡 51.85% | 4.1481 | 315 ms | 255 | ✓ 616KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 45 ms | 14 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 77 ms | 49 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698040) | 🟡 53.34% | 4.2669 | 316 ms | 35 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700412) | 🟡 53.52% | 4.2814 | 15 ms | 4 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 542KiB (554460) | 🟢 47.44% | 3.7952 | 741 ms | 775 | ✓ 600KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 31 ms | 8 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 62 ms | 31 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207576) | 🟢 17.76% | 1.4208 | 231 ms | 16 | ✓ 939KiB saved | ✓ |
| zstd | 265KiB (270395) | 🟢 23.14% | 1.8508 | 9 ms | 5 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 895KiB (915495) | 🟢 45.77% | 3.6620 | 1218 ms | 1320 | ✓ 1.1MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 56 ms | 16 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 93 ms | 46 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494416) | 🟢 24.72% | 1.9777 | 461 ms | 25 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (624404) | 🟢 31.22% | 2.4976 | 16 ms | 6 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 6 ms | 5 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 43 ms | 10 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 138 ms | 88 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000160) | 🔴 100.01% | 8.0006 | 370 ms | 6 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 6 ms | 5 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 874KiB (894847) | 🔴 88.41% | 7.0731 | 12 ms | 40 | ✓ 115KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 38 ms | 9 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 71 ms | 48 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868756) | 🔴 85.84% | 6.8669 | 190 ms | 42 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895471) | 🔴 88.48% | 7.0780 | 7 ms | 5 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1.8MiB (1846298) | 🔴 88.04% | 7.0431 | 24 ms | 79 | ✓ 245KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 61 ms | 17 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 124 ms | 85 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727568) | 🔴 82.38% | 6.5901 | 420 ms | 84 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1840935) | 🔴 87.78% | 7.0226 | 11 ms | 6 | ✓ 251KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 711KiB (727858) | 🟢 28.81% | 2.3045 | 3729 ms | 3512 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 191 ms | 18 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 134 ms | 69 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750600) | 🟢 29.71% | 2.3765 | 912 ms | 30 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905085) | 🟢 35.82% | 2.8656 | 17 ms | 7 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 610KiB (623733) | 🟡 60.99% | 4.8788 | 4592 ms | 5307 | ✓ 390KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 42 ms | 10 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 53 ms | 36 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366908) | 🟢 35.87% | 2.8699 | 215 ms | 20 | ✓ 641KiB saved | ✓ |
| zstd | 450KiB (459851) | 🟢 44.96% | 3.5969 | 10 ms | 6 | ✓ 550KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| zstd | 11 ms | 🥇 #1 |
| gzip | 63 ms | 🥈 #2 |
| bzip2 | 94 ms | 🥉 #3 |
| xz | 389 ms |  #4 |
| g07 | 1329 ms |  #5 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 5 ms | 🥇 #1 |
| gzip | 12 ms | 🥈 #2 |
| xz | 32 ms | 🥉 #3 |
| bzip2 | 56 ms |  #4 |
| g07 | 1411 ms |  #5 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 4 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **zstd** | 58.59% | 0 | 7.4MiB | ⭐ |
|  4 | **gzip** | 59.47% | 0 | 7.5MiB |  |
|  5 | **g07** | 62.74% | 2 | 7.9MiB |  |

### Files Won by Each Tool

- **g07**: 2 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 4 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07 | 13MiB | 7.9MiB | 62.74% | 5.0190 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3324 | 8 |
| zstd | 13MiB | 7.4MiB | 58.59% | 4.6872 | 8 |

---

*Report generated on Fri Mar  6 12:37:15 AM WET 2026*
