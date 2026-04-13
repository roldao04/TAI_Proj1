# Comprehensive Compression Benchmark Results

**Project**: TAI - Algorithmic Information Theory
**Date**: 2026-04-13
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

| File | Size | g07-v10 | g07-v5 | g07-v6 | g07-v7 | g07-v8 | g07-v9 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|----------|----------|----------|----------|----------|------|-------|----|----|--------|
| A | 1.3MiB | **51.84%** ⭐ | 53.30% | 59.53% | 53.57% | 87.97% | 51.84% | 58.49% | 54.05% | 53.34% | 53.52% | g07-v10 |
| B | 1.2MiB | **17.35%** ⭐ | 17.35% | 19.58% | 21.90% | 37.84% | 17.35% | 22.89% | 17.92% | 17.76% | 23.11% | g07-v10 |
| C | 2.0MiB | 25.93% | 25.93% | 28.27% | 32.90% | 45.22% | 25.93% | 32.57% | 26.51% | **24.72%** ⭐ | 31.19% | xz |
| D | 2.0MiB | **0.00%** ⭐ | 100.00% | 100.01% | 100.00% | 100.00% | 100.00% | 100.02% | 100.45% | 100.01% | 100.00% | g07-v10 |
| E | 989KiB | **85.60%** ⭐ | 86.03% | 97.46% | 88.22% | 100.00% | 85.60% | 88.66% | 88.43% | 85.84% | 88.53% | g07-v10 |
| F | 2.0MiB | 81.70% | 82.42% | 84.09% | 87.65% | 100.00% | 81.70% | 88.01% | **81.06%** ⭐ | 82.38% | 87.81% | bzip2 |
| G | 2.5MiB | **28.48%** ⭐ | 28.82% | 34.34% | 39.20% | 72.76% | 28.48% | 36.99% | 28.70% | 29.71% | 35.82% | g07-v10 |
| H | 999KiB | 43.64% | 43.79% | 47.07% | 47.09% | 58.02% | 43.64% | 44.02% | 42.34% | **35.88%** ⭐ | 44.86% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 663KiB (678467) | 🟡 51.84% | 4.1472 | 325 ms | 73 | ✓ 616KiB saved | ✓ |
| g07-v5 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 103 ms | 85 | ✓ 597KiB saved | ✓ |
| g07-v6 | 761KiB (779167) | 🟡 59.53% | 4.7628 | 6897 ms | 6709 | ✓ 518KiB saved | ✓ |
| g07-v7 | 685KiB (701048) | 🟡 53.57% | 4.2852 | 31 ms | 29 | ✓ 594KiB saved | ✓ |
| g07-v8 | 1.1MiB (1151299) | 🔴 87.97% | 7.0375 | 19 ms | 18 | ✓ 154KiB saved | ✓ |
| g07-v9 | 663KiB (678467) | 🟡 51.84% | 4.1472 | 330 ms | 76 | ✓ 616KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 58 ms | 25 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 110 ms | 67 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698048) | 🟡 53.34% | 4.2669 | 321 ms | 35 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700409) | 🟡 53.52% | 4.2813 | 23 ms | 15 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 451 ms | 45 | ✓ 944KiB saved | ✓ |
| g07-v5 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 65 ms | 44 | ✓ 944KiB saved | ✓ |
| g07-v6 | 224KiB (228807) | 🟢 19.58% | 1.5661 | 1492 ms | 1347 | ✓ 918KiB saved | ✓ |
| g07-v7 | 251KiB (256016) | 🟢 21.90% | 1.7524 | 24 ms | 24 | ✓ 892KiB saved | ✓ |
| g07-v8 | 432KiB (442203) | 🟢 37.84% | 3.0268 | 22 ms | 8 | ✓ 710KiB saved | ✓ |
| g07-v9 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 431 ms | 45 | ✓ 944KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 42 ms | 15 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 94 ms | 35 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207584) | 🟢 17.76% | 1.4209 | 255 ms | 20 | ✓ 939KiB saved | ✓ |
| zstd | 264KiB (270069) | 🟢 23.11% | 1.8486 | 22 ms | 15 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 678 ms | 69 | ✓ 1.5MiB saved | ✓ |
| g07-v5 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 94 ms | 78 | ✓ 1.5MiB saved | ✓ |
| g07-v6 | 553KiB (565499) | 🟢 28.27% | 2.2620 | 4638 ms | 4250 | ✓ 1.4MiB saved | ✓ |
| g07-v7 | 643KiB (657947) | 🟢 32.90% | 2.6318 | 28 ms | 25 | ✓ 1.3MiB saved | ✓ |
| g07-v8 | 884KiB (904314) | 🟢 45.22% | 3.6173 | 21 ms | 22 | ✓ 1.1MiB saved | ✓ |
| g07-v9 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 677 ms | 68 | ✓ 1.5MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 66 ms | 17 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 126 ms | 70 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494424) | 🟢 24.72% | 1.9777 | 430 ms | 34 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (623877) | 🟢 31.19% | 2.4955 | 32 ms | 20 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 14B (14) | 🟢 0.00% | 0.0001 | 20 ms | 28 | ✓ 2.0MiB saved | ✓ |
| g07-v5 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 17 ms | 16 | ✗  overhead | ✓ |
| g07-v6 | 2.0MiB (2000138) | 🔴 100.01% | 8.0006 | 19364 ms | 18773 | ✗  overhead | ✓ |
| g07-v7 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 6 ms | 13 | ✗  overhead | ✓ |
| g07-v8 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 17 ms | 15 | ✗  overhead | ✓ |
| g07-v9 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 18 ms | 16 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 65 ms | 9 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 171 ms | 110 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000168) | 🔴 100.01% | 8.0007 | 320 ms | 9 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 15 ms | 14 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 847KiB (866359) | 🔴 85.60% | 6.8479 | 637 ms | 181 | ✓ 143KiB saved | ✓ |
| g07-v5 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 67 ms | 136 | ✓ 139KiB saved | ✓ |
| g07-v6 | 964KiB (986409) | 🔴 97.46% | 7.7968 | 7781 ms | 7238 | ✓ 26KiB saved | ✓ |
| g07-v7 | 872KiB (892886) | 🔴 88.22% | 7.0576 | 10 ms | 17 | ✓ 117KiB saved | ✓ |
| g07-v8 | 989KiB (1012121) | 🔴 100.00% | 8.0001 | 12 ms | 13 | ✗  overhead | ✓ |
| g07-v9 | 847KiB (866359) | 🔴 85.60% | 6.8479 | 658 ms | 178 | ✓ 143KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 49 ms | 14 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 97 ms | 62 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868764) | 🔴 85.84% | 6.8669 | 158 ms | 41 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895975) | 🔴 88.53% | 7.0820 | 12 ms | 14 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 1.7MiB (1713386) | 🔴 81.70% | 6.5360 | 1032 ms | 213 | ✓ 375KiB saved | ✓ |
| g07-v5 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 42 ms | 82 | ✓ 360KiB saved | ✓ |
| g07-v6 | 1.7MiB (1763443) | 🔴 84.09% | 6.7270 | 17327 ms | 16178 | ✓ 326KiB saved | ✓ |
| g07-v7 | 1.8MiB (1838063) | 🔴 87.65% | 7.0117 | 14 ms | 21 | ✓ 254KiB saved | ✓ |
| g07-v8 | 2.1MiB (2097161) | 🔴 100.00% | 8.0000 | 17 ms | 14 | ✗  overhead | ✓ |
| g07-v9 | 1.7MiB (1713386) | 🔴 81.70% | 6.5360 | 1022 ms | 214 | ✓ 375KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 73 ms | 23 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 173 ms | 110 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727576) | 🔴 82.38% | 6.5902 | 346 ms | 72 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1841432) | 🔴 87.81% | 7.0245 | 12 ms | 12 | ✓ 250KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 703KiB (719531) | 🟢 28.48% | 2.2781 | 317 ms | 39 | ✓ 1.8MiB saved | ✓ |
| g07-v5 | 712KiB (728184) | 🟢 28.82% | 2.3055 | 91 ms | 51 | ✓ 1.8MiB saved | ✓ |
| g07-v6 | 848KiB (867615) | 🟢 34.34% | 2.7470 | 8679 ms | 8234 | ✓ 1.6MiB saved | ✓ |
| g07-v7 | 968KiB (990555) | 🟢 39.20% | 3.1362 | 31 ms | 27 | ✓ 1.5MiB saved | ✓ |
| g07-v8 | 1.8MiB (1838351) | 🟡 72.76% | 5.8204 | 17 ms | 17 | ✓ 673KiB saved | ✓ |
| g07-v9 | 703KiB (719531) | 🟢 28.48% | 2.2781 | 325 ms | 43 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 237 ms | 18 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 161 ms | 81 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750608) | 🟢 29.71% | 2.3765 | 843 ms | 41 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905141) | 🟢 35.82% | 2.8658 | 30 ms | 13 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v10 | 436KiB (446377) | 🟢 43.64% | 3.4915 | 584 ms | 61 | ✓ 563KiB saved | ✓ |
| g07-v5 | 438KiB (447874) | 🟢 43.79% | 3.5033 | 97 ms | 103 | ✓ 562KiB saved | ✓ |
| g07-v6 | 471KiB (481460) | 🟢 47.07% | 3.7660 | 3945 ms | 3717 | ✓ 529KiB saved | ✓ |
| g07-v7 | 471KiB (481597) | 🟢 47.09% | 3.7670 | 33 ms | 32 | ✓ 529KiB saved | ✓ |
| g07-v8 | 580KiB (593367) | 🟡 58.02% | 4.6413 | 23 ms | 20 | ✓ 420KiB saved | ✓ |
| g07-v9 | 436KiB (446377) | 🟢 43.64% | 3.4915 | 594 ms | 59 | ✓ 563KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 54 ms | 13 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 79 ms | 46 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366916) | 🟢 35.88% | 2.8700 | 225 ms | 32 | ✓ 641KiB saved | ✓ |
| zstd | 449KiB (458829) | 🟢 44.86% | 3.5889 | 25 ms | 11 | ✓ 551KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| g07-v8 | 18 ms | 🥇 #1 |
| zstd | 21 ms | 🥈 #2 |
| g07-v7 | 22 ms | 🥉 #3 |
| g07-v5 | 72 ms |  #4 |
| gzip | 80 ms |  #5 |
| bzip2 | 126 ms |  #6 |
| xz | 362 ms |  #7 |
| g07-v10 | 505 ms |  #8 |
| g07-v9 | 506 ms |  #9 |
| g07-v6 | 8765 ms |  #10 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 14 ms | 🥇 #1 |
| g07-v8 | 15 ms | 🥈 #2 |
| gzip | 16 ms | 🥉 #3 |
| g07-v7 | 23 ms |  #4 |
| xz | 35 ms |  #5 |
| bzip2 | 72 ms |  #6 |
| g07-v5 | 74 ms |  #7 |
| g07-v9 | 87 ms |  #8 |
| g07-v10 | 88 ms |  #9 |
| g07-v6 | 8305 ms |  #10 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **g07-v10** | 39.17% | 5 | 5.0MiB | ⭐⭐⭐ |
| 🥈 2 | **xz** | 54.16% | 2 | 6.8MiB | ⭐⭐ |
| 🥉 3 | **g07-v9** | 54.40% | 0 | 6.9MiB | ⭐ |
|  4 | **g07-v5** | 54.77% | 0 | 6.9MiB |  |
|  5 | **bzip2** | 54.88% | 1 | 6.9MiB |  |
|  6 | **g07-v6** | 58.41% | 0 | 7.4MiB |  |
|  7 | **zstd** | 58.58% | 0 | 7.4MiB |  |
|  8 | **gzip** | 59.47% | 0 | 7.5MiB |  |
|  9 | **g07-v7** | 59.52% | 0 | 7.5MiB |  |
|  10 | **g07-v8** | 76.42% | 0 | 9.6MiB |  |

### Files Won by Each Tool

- **g07-v10**: 5 file(s)
- **g07-v5**: 0 file(s)
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
| g07-v5 | 13MiB | 6.9MiB | 54.77% | 4.3814 | 8 |
| g07-v6 | 13MiB | 7.4MiB | 58.41% | 4.6726 | 8 |
| g07-v7 | 13MiB | 7.5MiB | 59.52% | 4.7612 | 8 |
| g07-v8 | 13MiB | 9.6MiB | 76.42% | 6.1136 | 8 |
| g07-v9 | 13MiB | 6.9MiB | 54.40% | 4.3516 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3325 | 8 |
| zstd | 13MiB | 7.4MiB | 58.58% | 4.6867 | 8 |

---

*Report generated on Mon Apr 13 02:41:58 PM UTC 2026*
