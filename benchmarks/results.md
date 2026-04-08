# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-04-08
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

| File | Size | g07-v5 | g07-v6 | g07-v7 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|----------|----------|------|-------|----|----|--------|
| A | 1.3MiB | 53.68% | TIMEOUT% | 53.39% | 58.49% | 54.05% | **53.34%** ⭐ | 53.52% | xz |
| B | 1.2MiB | 18.05% | TIMEOUT% | 20.62% | 22.89% | 17.92% | **17.76%** ⭐ | 23.11% | xz |
| C | 2.0MiB | 26.65% | TIMEOUT% | 31.94% | 32.57% | 26.51% | **24.72%** ⭐ | 31.19% | xz |
| D | 2.0MiB | **100.00%** ⭐ | TIMEOUT% | 100.00% | 100.02% | 100.45% | 100.01% | 100.00% | g07-v5 |
| E | 989KiB | 86.03% | TIMEOUT% | 88.17% | 88.66% | 88.43% | **85.84%** ⭐ | 88.53% | xz |
| F | 2.0MiB | 82.42% | TIMEOUT% | 87.62% | 88.01% | **81.06%** ⭐ | 82.38% | 87.81% | bzip2 |
| G | 2.5MiB | 29.90% | TIMEOUT% | 39.24% | 36.99% | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | 47.35% | TIMEOUT% | 46.93% | 44.02% | 42.34% | **35.88%** ⭐ | 44.86% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 687KiB (702572) | 🟡 53.68% | 4.2946 | 119 ms | 83 | ✓ 592KiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 683KiB (698690) | 🟡 53.39% | 4.2708 | 87 ms | 42 | ✓ 596KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 64 ms | 24 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 114 ms | 60 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698048) | 🟡 53.34% | 4.2669 | 353 ms | 36 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700409) | 🟡 53.52% | 4.2813 | 23 ms | 8 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 207KiB (210953) | 🟢 18.05% | 1.4439 | 55 ms | 46 | ✓ 936KiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 236KiB (240989) | 🟢 20.62% | 1.6495 | 43 ms | 31 | ✓ 907KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 35 ms | 9 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 89 ms | 31 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207584) | 🟢 17.76% | 1.4209 | 261 ms | 17 | ✓ 939KiB saved | ✓ |
| zstd | 264KiB (270069) | 🟢 23.11% | 1.8486 | 23 ms | 12 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 521KiB (532968) | 🟢 26.65% | 2.1319 | 109 ms | 90 | ✓ 1.4MiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 624KiB (638869) | 🟢 31.94% | 2.5555 | 73 ms | 50 | ✓ 1.3MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 63 ms | 14 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 115 ms | 57 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494424) | 🟢 24.72% | 1.9777 | 489 ms | 26 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (623877) | 🟢 31.19% | 2.4955 | 17 ms | 17 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 15 ms | 13 | ✗  overhead | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 14 ms | 13 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 66 ms | 11 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 185 ms | 111 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000168) | 🔴 100.01% | 8.0007 | 304 ms | 11 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 13 ms | 12 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 80 ms | 142 | ✓ 139KiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 872KiB (892377) | 🔴 88.17% | 7.0536 | 13 ms | 11 | ✓ 117KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 47 ms | 10 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 88 ms | 59 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868764) | 🔴 85.84% | 6.8669 | 164 ms | 34 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895975) | 🔴 88.53% | 7.0820 | 9 ms | 12 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 49 ms | 71 | ✓ 360KiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 1.8MiB (1837503) | 🔴 87.62% | 7.0095 | 31 ms | 31 | ✓ 254KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 86 ms | 18 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 155 ms | 111 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727576) | 🔴 82.38% | 6.5902 | 345 ms | 63 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1841432) | 🔴 87.81% | 7.0245 | 11 ms | 16 | ✓ 250KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 738KiB (755544) | 🟢 29.90% | 2.3921 | 97 ms | 65 | ✓ 1.7MiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 969KiB (991593) | 🟢 39.24% | 3.1395 | 146 ms | 63 | ✓ 1.5MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 247 ms | 19 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 167 ms | 86 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750608) | 🟢 29.71% | 2.3765 | 811 ms | 41 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905141) | 🟢 35.82% | 2.8658 | 36 ms | 18 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 473KiB (484327) | 🟢 47.35% | 3.7884 | 119 ms | 154 | ✓ 526KiB saved | ✓ |
| g07-v6 | ⏱️ TIMEOUT (>60s) | - | - | - | - | - | ⏱️ |
| g07-v7 | 469KiB (479971) | 🟢 46.93% | 3.7543 | 62 ms | 37 | ✓ 531KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 60 ms | 12 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 69 ms | 46 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366916) | 🟢 35.88% | 2.8700 | 217 ms | 24 | ✓ 641KiB saved | ✓ |
| zstd | 449KiB (458829) | 🟢 44.86% | 3.5889 | 20 ms | 13 | ✓ 551KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| zstd | 19 ms | 🥇 #1 |
| g07-v7 | 58 ms | 🥈 #2 |
| g07-v5 | 80 ms | 🥉 #3 |
| gzip | 83 ms |  #4 |
| bzip2 | 122 ms |  #5 |
| xz | 368 ms |  #6 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 13 ms | 🥇 #1 |
| gzip | 14 ms | 🥈 #2 |
| xz | 31 ms | 🥉 #3 |
| g07-v7 | 34 ms |  #4 |
| bzip2 | 70 ms |  #5 |
| g07-v5 | 83 ms |  #6 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 5 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **g07-v5** | 55.46% | 1 | 7.0MiB | ⭐ |
|  4 | **zstd** | 58.58% | 0 | 7.4MiB |  |
|  5 | **g07-v7** | 59.23% | 0 | 7.5MiB |  |
|  6 | **gzip** | 59.47% | 0 | 7.5MiB |  |

### Files Won by Each Tool

- **g07-v5**: 1 file(s)
- **g07-v6**: 0 file(s)
- **g07-v7**: 0 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 5 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07-v5 | 13MiB | 7.0MiB | 55.46% | 4.4370 | 8 |
| g07-v7 | 13MiB | 7.5MiB | 59.23% | 4.7380 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3325 | 8 |
| zstd | 13MiB | 7.4MiB | 58.58% | 4.6867 | 8 |

---

*Report generated on Wed Apr  8 06:47:54 PM UTC 2026*
