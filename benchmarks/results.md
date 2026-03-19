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
| A | 1.3MiB | **53.43%** ⭐ | 58.49% | 54.05% | **53.34%** ⭐ | 53.52% | xz |
| B | 1.2MiB | **18.91%** ⭐ | 22.89% | **17.92%** ⭐ | **17.76%** ⭐ | 23.11% | xz |
| C | 2.0MiB | **27.47%** ⭐ | 32.57% | **26.51%** ⭐ | **24.72%** ⭐ | 31.19% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.02% | 100.45% | 100.01% | 100.00% | g07 |
| E | 989KiB | **88.41%** ⭐ | 88.66% | 88.43% | **85.84%** ⭐ | 88.53% | xz |
| F | 2.0MiB | **88.04%** ⭐ | **88.01%** ⭐ | **81.06%** ⭐ | 82.38% | 87.81% | bzip2 |
| G | 2.5MiB | **29.28%** ⭐ | 36.99% | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | **45.88%** ⭐ | **44.02%** ⭐ | **42.34%** ⭐ | **35.88%** ⭐ | 44.86% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 683KiB (699230) | 🟡 53.43% | 4.2741 | 88 ms | 63 | ✓ 596KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 67 ms | 16 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 110 ms | 60 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698048) | 🟡 53.34% | 4.2669 | 306 ms | 38 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700409) | 🟡 53.52% | 4.2813 | 26 ms | 14 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 216KiB (221002) | 🟢 18.91% | 1.5127 | 58 ms | 43 | ✓ 926KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 41 ms | 9 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 97 ms | 31 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207584) | 🟢 17.76% | 1.4209 | 251 ms | 16 | ✓ 939KiB saved | ✓ |
| zstd | 264KiB (270069) | 🟢 23.11% | 1.8486 | 24 ms | 12 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 537KiB (549348) | 🟢 27.47% | 2.1974 | 60 ms | 48 | ✓ 1.4MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 71 ms | 15 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 125 ms | 60 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494424) | 🟢 24.72% | 1.9777 | 443 ms | 30 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (623877) | 🟢 31.19% | 2.4955 | 24 ms | 17 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 16 ms | 14 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 73 ms | 11 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 182 ms | 106 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000168) | 🔴 100.01% | 8.0007 | 294 ms | 10 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 15 ms | 12 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 874KiB (894847) | 🔴 88.41% | 7.0731 | 30 ms | 66 | ✓ 115KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 44 ms | 16 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 100 ms | 61 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868764) | 🔴 85.84% | 6.8669 | 172 ms | 37 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895975) | 🔴 88.53% | 7.0820 | 10 ms | 13 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1.8MiB (1846298) | 🔴 88.04% | 7.0431 | 42 ms | 113 | ✓ 245KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 80 ms | 19 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 159 ms | 112 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727576) | 🔴 82.38% | 6.5902 | 367 ms | 78 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1841432) | 🔴 87.81% | 7.0245 | 11 ms | 15 | ✓ 250KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 723KiB (739794) | 🟢 29.28% | 2.3423 | 74 ms | 43 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 252 ms | 18 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 174 ms | 82 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750608) | 🟢 29.71% | 2.3765 | 891 ms | 37 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905141) | 🟢 35.82% | 2.8658 | 32 ms | 16 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 459KiB (469251) | 🟢 45.88% | 3.6705 | 104 ms | 97 | ✓ 541KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 62 ms | 13 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 79 ms | 51 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366916) | 🟢 35.88% | 2.8700 | 217 ms | 25 | ✓ 641KiB saved | ✓ |
| zstd | 449KiB (458829) | 🟢 44.86% | 3.5889 | 18 ms | 14 | ✓ 551KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| zstd | 20 ms | 🥇 #1 |
| g07 | 59 ms | 🥈 #2 |
| gzip | 86 ms | 🥉 #3 |
| bzip2 | 128 ms |  #4 |
| xz | 367 ms |  #5 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| gzip | 14 ms | 🥇 #1 |
| zstd | 14 ms | 🥈 #2 |
| xz | 33 ms | 🥉 #3 |
| g07 | 60 ms |  #4 |
| bzip2 | 70 ms |  #5 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 5 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **g07** | 56.48% | 1 | 7.1MiB | ⭐ |
|  4 | **zstd** | 58.58% | 0 | 7.4MiB |  |
|  5 | **gzip** | 59.47% | 0 | 7.5MiB |  |

### Files Won by Each Tool

- **g07**: 1 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 5 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07 | 13MiB | 7.1MiB | 56.48% | 4.5186 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3325 | 8 |
| zstd | 13MiB | 7.4MiB | 58.58% | 4.6867 | 8 |

---

*Report generated on Thu Mar 19 12:10:30 AM UTC 2026*
