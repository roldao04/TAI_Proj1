# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-05-29
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

| File | Size | g07-v10 | g07-v6 | g07-v7 | g07-v8 | g07-v9 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|----------|----------|----------|----------|------|-------|----|----|--------|
| A | 1.3MiB | **51.84%** ⭐ | 51.84% | 53.57% | 87.97% | 53.30% | 58.49% | 54.05% | 53.34% | 53.52% | g07-v10 |
| B | 1.2MiB | **17.35%** ⭐ | 17.35% | 21.90% | 37.84% | 17.35% | 22.89% | 17.92% | 17.76% | 23.14% | g07-v10 |
| C | 2.0MiB | 25.93% | 25.93% | 32.90% | 45.22% | 25.93% | 32.57% | 26.51% | **24.72%** ⭐ | 31.22% | xz |
| D | 2.0MiB | **0.00%** ⭐ | 100.00% | 100.00% | 100.00% | 100.00% | 100.02% | 100.45% | 100.01% | 100.00% | g07-v10 |
| E | 989KiB | **85.60%** ⭐ | 85.60% | 88.22% | 100.00% | 86.03% | 88.66% | 88.43% | 85.84% | 88.48% | g07-v10 |
| F | 2.0MiB | 81.70% | 81.70% | 87.65% | 100.00% | 82.42% | 88.01% | **81.06%** ⭐ | 82.38% | 87.78% | bzip2 |
| G | 2.5MiB | **28.48%** ⭐ | 28.48% | 39.20% | 72.76% | 28.82% | 36.99% | 28.70% | 29.71% | 35.82% | g07-v10 |
| H | 999KiB | 43.64% | 43.64% | 47.09% | 58.02% | 43.79% | 44.02% | 42.34% | **35.87%** ⭐ | 44.96% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 663KiB (678467) | 🟡 51.84% | 4.1472 | 857 ms | 177 | ✓ 616KiB saved | ✓ |
| g07-v6 | 663KiB (678467) | 🟡 51.84% | 4.1472 | 852 ms | 171 | ✓ 616KiB saved | ✓ |
| g07-v7 | 685KiB (701048) | 🟡 53.57% | 4.2852 | 70 ms | 42 | ✓ 594KiB saved | ✓ |
| g07-v8 | 1.1MiB (1151299) | 🔴 87.97% | 7.0375 | 25 ms | 17 | ✓ 154KiB saved | ✓ |
| g07-v9 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 251 ms | 184 | ✓ 597KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 138 ms | 38 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 221 ms | 156 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698040) | 🟡 53.34% | 4.2669 | 983 ms | 96 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700412) | 🟡 53.52% | 4.2814 | 44 ms | 14 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 1241 ms | 98 | ✓ 944KiB saved | ✓ |
| g07-v6 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 1201 ms | 99 | ✓ 944KiB saved | ✓ |
| g07-v7 | 251KiB (256016) | 🟢 21.90% | 1.7524 | 49 ms | 32 | ✓ 892KiB saved | ✓ |
| g07-v8 | 432KiB (442203) | 🟢 37.84% | 3.0268 | 18 ms | 16 | ✓ 710KiB saved | ✓ |
| g07-v9 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 129 ms | 96 | ✓ 944KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 89 ms | 23 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 187 ms | 81 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207576) | 🟢 17.76% | 1.4208 | 683 ms | 36 | ✓ 939KiB saved | ✓ |
| zstd | 265KiB (270395) | 🟢 23.14% | 1.8508 | 24 ms | 14 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 1848 ms | 184 | ✓ 1.5MiB saved | ✓ |
| g07-v6 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 1821 ms | 192 | ✓ 1.5MiB saved | ✓ |
| g07-v7 | 643KiB (657947) | 🟢 32.90% | 2.6318 | 61 ms | 42 | ✓ 1.3MiB saved | ✓ |
| g07-v8 | 884KiB (904314) | 🟢 45.22% | 3.6173 | 30 ms | 23 | ✓ 1.1MiB saved | ✓ |
| g07-v9 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 239 ms | 189 | ✓ 1.5MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 165 ms | 40 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 274 ms | 143 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494416) | 🟢 24.72% | 1.9777 | 1283 ms | 73 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (624404) | 🟢 31.22% | 2.4976 | 35 ms | 16 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 14B (14) | 🟢 0.00% | 0.0001 | 16 ms | 22 | ✓ 2.0MiB saved | ✓ |
| g07-v6 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 17 ms | 14 | ✗  overhead | ✓ |
| g07-v7 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 16 ms | 15 | ✗  overhead | ✓ |
| g07-v8 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 15 ms | 12 | ✗  overhead | ✓ |
| g07-v9 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 15 ms | 14 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 133 ms | 28 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 401 ms | 307 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000160) | 🔴 100.01% | 8.0006 | 852 ms | 18 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 16 ms | 13 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 847KiB (866359) | 🔴 85.60% | 6.8479 | 1616 ms | 440 | ✓ 143KiB saved | ✓ |
| g07-v6 | 847KiB (866359) | 🔴 85.60% | 6.8479 | 1525 ms | 418 | ✓ 143KiB saved | ✓ |
| g07-v7 | 872KiB (892886) | 🔴 88.22% | 7.0576 | 21 ms | 20 | ✓ 117KiB saved | ✓ |
| g07-v8 | 989KiB (1012121) | 🔴 100.00% | 8.0001 | 13 ms | 12 | ✗  overhead | ✓ |
| g07-v9 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 155 ms | 368 | ✓ 139KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 107 ms | 25 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 219 ms | 153 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868756) | 🔴 85.84% | 6.8669 | 443 ms | 133 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895471) | 🔴 88.48% | 7.0780 | 18 ms | 13 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 1.7MiB (1713386) | 🔴 81.70% | 6.5360 | 2625 ms | 506 | ✓ 375KiB saved | ✓ |
| g07-v6 | 1.7MiB (1713386) | 🔴 81.70% | 6.5360 | 2434 ms | 497 | ✓ 375KiB saved | ✓ |
| g07-v7 | 1.8MiB (1838063) | 🔴 87.65% | 7.0117 | 25 ms | 24 | ✓ 254KiB saved | ✓ |
| g07-v8 | 2.1MiB (2097161) | 🔴 100.00% | 8.0000 | 15 ms | 14 | ✗  overhead | ✓ |
| g07-v9 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 92 ms | 192 | ✓ 360KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 185 ms | 45 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 385 ms | 274 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727568) | 🔴 82.38% | 6.5901 | 974 ms | 252 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1840935) | 🔴 87.78% | 7.0226 | 26 ms | 15 | ✓ 251KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 703KiB (719531) | 🟢 28.48% | 2.2781 | 948 ms | 90 | ✓ 1.8MiB saved | ✓ |
| g07-v6 | 703KiB (719531) | 🟢 28.48% | 2.2781 | 920 ms | 89 | ✓ 1.8MiB saved | ✓ |
| g07-v7 | 968KiB (990555) | 🟢 39.20% | 3.1362 | 89 ms | 43 | ✓ 1.5MiB saved | ✓ |
| g07-v8 | 1.8MiB (1838351) | 🟡 72.76% | 5.8204 | 29 ms | 22 | ✓ 673KiB saved | ✓ |
| g07-v9 | 712KiB (728184) | 🟢 28.82% | 2.3055 | 236 ms | 132 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 577 ms | 47 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 386 ms | 222 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750600) | 🟢 29.71% | 2.3765 | 2546 ms | 91 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905085) | 🟢 35.82% | 2.8656 | 45 ms | 18 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 436KiB (446377) | 🟢 43.64% | 3.4915 | 1591 ms | 136 | ✓ 563KiB saved | ✓ |
| g07-v6 | 436KiB (446377) | 🟢 43.64% | 3.4915 | 1548 ms | 136 | ✓ 563KiB saved | ✓ |
| g07-v7 | 471KiB (481597) | 🟢 47.09% | 3.7670 | 77 ms | 43 | ✓ 529KiB saved | ✓ |
| g07-v8 | 580KiB (593367) | 🟡 58.02% | 4.6413 | 20 ms | 14 | ✓ 420KiB saved | ✓ |
| g07-v9 | 438KiB (447874) | 🟢 43.79% | 3.5033 | 236 ms | 236 | ✓ 562KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 124 ms | 26 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 156 ms | 106 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366908) | 🟢 35.87% | 2.8699 | 582 ms | 59 | ✓ 641KiB saved | ✓ |
| zstd | 450KiB (459851) | 🟢 44.96% | 3.5969 | 26 ms | 14 | ✓ 550KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| g07-v8 | 20 ms | 🥇 #1 |
| zstd | 29 ms | 🥈 #2 |
| g07-v7 | 51 ms | 🥉 #3 |
| g07-v9 | 169 ms |  #4 |
| gzip | 189 ms |  #5 |
| bzip2 | 278 ms |  #6 |
| xz | 1043 ms |  #7 |
| g07-v6 | 1289 ms |  #8 |
| g07-v10 | 1342 ms |  #9 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 14 ms | 🥇 #1 |
| g07-v8 | 16 ms | 🥈 #2 |
| g07-v7 | 32 ms | 🥉 #3 |
| gzip | 34 ms |  #4 |
| xz | 94 ms |  #5 |
| g07-v9 | 176 ms |  #6 |
| bzip2 | 180 ms |  #7 |
| g07-v6 | 202 ms |  #8 |
| g07-v10 | 206 ms |  #9 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **g07-v10** | 39.17% | 5 | 5.0MiB | ⭐⭐⭐ |
| 🥈 2 | **xz** | 54.16% | 2 | 6.8MiB | ⭐⭐ |
| 🥉 3 | **g07-v6** | 54.40% | 0 | 6.9MiB | ⭐ |
|  4 | **g07-v9** | 54.77% | 0 | 6.9MiB |  |
|  5 | **bzip2** | 54.88% | 1 | 6.9MiB |  |
|  6 | **zstd** | 58.59% | 0 | 7.4MiB |  |
|  7 | **gzip** | 59.47% | 0 | 7.5MiB |  |
|  8 | **g07-v7** | 59.52% | 0 | 7.5MiB |  |
|  9 | **g07-v8** | 76.42% | 0 | 9.6MiB |  |

### Files Won by Each Tool

- **g07-v10**: 5 file(s)
- **g07-v6**: 0 file(s)
- **g07-v7**: 0 file(s)
- **g07-v8**: 0 file(s)
- **g07-v9**: 0 file(s)
- **gzip**: 0 file(s)
- **bzip2**: 1 file(s)
- **xz**: 2 file(s)
- **zstd**: 0 file(s)

---

## Overall Statistics

| Tool | Total Original | Total Compressed | Average Ratio | Bits/Symbol | Files Tested |
|------|---------------|------------------|---------------|-------------|--------------|
| g07-v10 | 13MiB | 5.0MiB | 39.17% | 3.1336 | 8 |
| g07-v6 | 13MiB | 6.9MiB | 54.40% | 4.3516 | 8 |
| g07-v7 | 13MiB | 7.5MiB | 59.52% | 4.7612 | 8 |
| g07-v8 | 13MiB | 9.6MiB | 76.42% | 6.1136 | 8 |
| g07-v9 | 13MiB | 6.9MiB | 54.77% | 4.3814 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3324 | 8 |
| zstd | 13MiB | 7.4MiB | 58.59% | 4.6872 | 8 |

---

*Report generated on Fri May 29 10:56:51 AM WEST 2026*
