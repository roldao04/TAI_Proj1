# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-02-26
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
| A | 1.3MiB | **52.05%** ⭐ | 58.49% | 54.05% | 53.34% | 53.52% | g07 |
| B | 1.2MiB | **67.34%** ⭐ | **22.89%** ⭐ | **17.92%** ⭐ | **17.76%** ⭐ | 23.14% | xz |
| C | 2.0MiB | **65.78%** ⭐ | **32.57%** ⭐ | **26.51%** ⭐ | **24.72%** ⭐ | 31.22% | xz |
| D | 2.0MiB | **100.05%** ⭐ | **100.02%** ⭐ | 100.45% | **100.01%** ⭐ | **100.00%** ⭐ | zstd |
| E | 989KiB | **88.22%** ⭐ | 88.66% | 88.43% | **85.84%** ⭐ | 88.48% | xz |
| F | 2.0MiB | **87.64%** ⭐ | 88.01% | **81.06%** ⭐ | 82.38% | 87.78% | bzip2 |
| G | 2.5MiB | **41.31%** ⭐ | **36.99%** ⭐ | **28.70%** ⭐ | 29.71% | 35.82% | bzip2 |
| H | 999KiB | **80.65%** ⭐ | **44.02%** ⭐ | **42.34%** ⭐ | **35.87%** ⭐ | 44.96% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 666KiB (681163) | 🟡 52.05% | 4.1637 | 44 ms | 63 ms | ✓ 613KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 46 ms | 12 ms | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 74 ms | 47 ms | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698040) | 🟡 53.34% | 4.2669 | 272 ms | 31 ms | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700412) | 🟡 53.52% | 4.2814 | 14 ms | 4 ms | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 769KiB (787009) | 🟡 67.34% | 5.3869 | 50 ms | 67 ms | ✓ 373KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 27 ms | 6 ms | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 59 ms | 25 ms | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207576) | 🟢 17.76% | 1.4208 | 207 ms | 10 ms | ✓ 939KiB saved | ✓ |
| zstd | 265KiB (270395) | 🟢 23.14% | 1.8508 | 7 ms | 3 ms | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1.3MiB (1315575) | 🟡 65.78% | 5.2623 | 77 ms | 118 ms | ✓ 669KiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 54 ms | 11 ms | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 102 ms | 50 ms | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494416) | 🟢 24.72% | 1.9777 | 415 ms | 23 ms | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (624404) | 🟢 31.22% | 2.4976 | 12 ms | 5 ms | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 2.0MiB (2001020) | 🔴 100.05% | 8.0041 | 99 ms | 160 ms | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 42 ms | 8 ms | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 135 ms | 82 ms | ✗  overhead | ✓ |
| xz | 2.0MiB (2000160) | 🔴 100.01% | 8.0006 | 291 ms | 4 ms | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 5 ms | 3 ms | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 872KiB (892855) | 🔴 88.22% | 7.0574 | 50 ms | 77 ms | ✓ 117KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 34 ms | 7 ms | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 67 ms | 46 ms | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868756) | 🔴 85.84% | 6.8669 | 165 ms | 42 ms | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895471) | 🔴 88.48% | 7.0780 | 6 ms | 3 ms | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1.8MiB (1837967) | 🔴 87.64% | 7.0113 | 100 ms | 148 ms | ✓ 254KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 57 ms | 15 ms | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 121 ms | 84 ms | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727568) | 🔴 82.38% | 6.5901 | 369 ms | 86 ms | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1840935) | 🔴 87.78% | 7.0226 | 10 ms | 4 ms | ✓ 251KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 1020KiB (1043822) | 🟢 41.31% | 3.3049 | 68 ms | 100 ms | ✓ 1.5MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 182 ms | 15 ms | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 124 ms | 69 ms | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750600) | 🟢 29.71% | 2.3765 | 748 ms | 31 ms | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905085) | 🟢 35.82% | 2.8656 | 18 ms | 7 ms | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07 | 806KiB (824865) | 🔴 80.65% | 6.4521 | 47 ms | 68 ms | ✓ 194KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 40 ms | 8 ms | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 50 ms | 36 ms | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366908) | 🟢 35.87% | 2.8699 | 182 ms | 21 ms | ✓ 641KiB saved | ✓ |
| zstd | 450KiB (459851) | 🟢 44.96% | 3.5969 | 10 ms | 4 ms | ✓ 550KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| zstd | 10 ms | 🥇 #1 |
| gzip | 60 ms | 🥈 #2 |
| g07 | 66 ms | 🥉 #3 |
| bzip2 | 91 ms |  #4 |
| xz | 331 ms |  #5 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 4 ms | 🥇 #1 |
| gzip | 10 ms | 🥈 #2 |
| xz | 31 ms | 🥉 #3 |
| bzip2 | 54 ms |  #4 |
| g07 | 100 ms |  #5 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 4 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **bzip2** | 54.88% | 2 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **zstd** | 58.59% | 1 | 7.4MiB | ⭐ |
|  4 | **gzip** | 59.47% | 0 | 7.5MiB |  |
|  5 | **g07** | 71.44% | 1 | 9.0MiB |  |

### Files Won by Each Tool

- **g07**: 1 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 2 file(s)
- **xz**: 4 file(s)
- **zstd**: 1 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07 | 13MiB | 9.0MiB | 71.44% | 5.7150 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3324 | 8 |
| zstd | 13MiB | 7.4MiB | 58.59% | 4.6872 | 8 |

---

*Report generated on Thu Feb 26 04:56:56 PM WET 2026*
