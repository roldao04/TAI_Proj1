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

| File | Size | g07-v5 | g07-v6 | g07-v7 | g07-v8 | g07-v9 | gzip | bzip2 | xz | zstd | Winner |
|------|------|----------|----------|----------|----------|----------|------|-------|----|----|--------|
| A | 1.3MiB | 53.30% | 59.53% | 53.57% | 87.97% | **51.84%** ⭐ | 58.49% | 54.05% | 53.34% | 53.52% | g07-v9 |
| B | 1.2MiB | **17.35%** ⭐ | 19.58% | 21.90% | 37.84% | 17.35% | 22.89% | 17.92% | 17.76% | 23.14% | g07-v5 |
| C | 2.0MiB | 25.93% | 28.27% | 32.90% | 45.22% | 25.93% | 32.57% | 26.51% | **24.72%** ⭐ | 31.22% | xz |
| D | 2.0MiB | **100.00%** ⭐ | 100.01% | 100.00% | 100.00% | 100.00% | 100.02% | 100.45% | 100.01% | 100.00% | g07-v5 |
| E | 989KiB | 86.03% | 97.46% | 88.22% | 100.00% | **85.60%** ⭐ | 88.66% | 88.43% | 85.84% | 88.48% | g07-v9 |
| F | 2.0MiB | 82.42% | 84.09% | 87.65% | 100.00% | 81.70% | 88.01% | **81.06%** ⭐ | 82.38% | 87.78% | bzip2 |
| G | 2.5MiB | 28.82% | 34.34% | 39.20% | 72.76% | **28.48%** ⭐ | 36.99% | 28.70% | 29.71% | 35.82% | g07-v9 |
| H | 999KiB | 43.79% | 47.07% | 47.09% | 58.02% | 43.64% | 44.02% | 42.34% | **35.87%** ⭐ | 44.96% | xz |

⭐ = Best compressor for that file

---

## Detailed Results by File

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 682KiB (697595) | 🟡 53.30% | 4.2641 | 100 ms | 92 | ✓ 597KiB saved | ✓ |
| g07-v6 | 761KiB (779167) | 🟡 59.53% | 4.7628 | 7202 ms | 6792 | ✓ 518KiB saved | ✓ |
| g07-v7 | 685KiB (701048) | 🟡 53.57% | 4.2852 | 31 ms | 22 | ✓ 594KiB saved | ✓ |
| g07-v8 | 1.1MiB (1151299) | 🔴 87.97% | 7.0375 | 18 ms | 12 | ✓ 154KiB saved | ✓ |
| g07-v9 | 663KiB (678467) | 🟡 51.84% | 4.1472 | 354 ms | 98 | ✓ 616KiB saved | ✓ |
| gzip | 748KiB (765497) | 🟡 58.49% | 4.6792 | 60 ms | 17 | ✓ 531KiB saved | ✓ |
| bzip2 | 691KiB (707422) | 🟡 54.05% | 4.3242 | 100 ms | 63 | ✓ 588KiB saved | ✓ |
| xz | 682KiB (698040) | 🟡 53.34% | 4.2669 | 333 ms | 46 | ✓ 597KiB saved | ✓ |
| zstd | 684KiB (700412) | 🟡 53.52% | 4.2814 | 16 ms | 11 | ✓ 595KiB saved | ✓ |

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 64 ms | 44 | ✓ 944KiB saved | ✓ |
| g07-v6 | 224KiB (228807) | 🟢 19.58% | 1.5661 | 1524 ms | 1321 | ✓ 918KiB saved | ✓ |
| g07-v7 | 251KiB (256016) | 🟢 21.90% | 1.7524 | 23 ms | 22 | ✓ 892KiB saved | ✓ |
| g07-v8 | 432KiB (442203) | 🟢 37.84% | 3.0268 | 12 ms | 13 | ✓ 710KiB saved | ✓ |
| g07-v9 | 199KiB (202758) | 🟢 17.35% | 1.3878 | 503 ms | 46 | ✓ 944KiB saved | ✓ |
| gzip | 262KiB (267569) | 🟢 22.89% | 1.8315 | 38 ms | 12 | ✓ 881KiB saved | ✓ |
| bzip2 | 205KiB (209479) | 🟢 17.92% | 1.4338 | 83 ms | 34 | ✓ 937KiB saved | ✓ |
| xz | 203KiB (207576) | 🟢 17.76% | 1.4208 | 266 ms | 17 | ✓ 939KiB saved | ✓ |
| zstd | 265KiB (270395) | 🟢 23.14% | 1.8508 | 16 ms | 9 | ✓ 878KiB saved | ✓ |

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 103 ms | 76 | ✓ 1.5MiB saved | ✓ |
| g07-v6 | 553KiB (565499) | 🟢 28.27% | 2.2620 | 4629 ms | 4200 | ✓ 1.4MiB saved | ✓ |
| g07-v7 | 643KiB (657947) | 🟢 32.90% | 2.6318 | 28 ms | 29 | ✓ 1.3MiB saved | ✓ |
| g07-v8 | 884KiB (904314) | 🟢 45.22% | 3.6173 | 22 ms | 16 | ✓ 1.1MiB saved | ✓ |
| g07-v9 | 507KiB (518678) | 🟢 25.93% | 2.0747 | 746 ms | 72 | ✓ 1.5MiB saved | ✓ |
| gzip | 637KiB (651482) | 🟢 32.57% | 2.6059 | 68 ms | 20 | ✓ 1.3MiB saved | ✓ |
| bzip2 | 518KiB (530253) | 🟢 26.51% | 2.1210 | 115 ms | 65 | ✓ 1.5MiB saved | ✓ |
| xz | 483KiB (494416) | 🟢 24.72% | 1.9777 | 449 ms | 34 | ✓ 1.5MiB saved | ✓ |
| zstd | 610KiB (624404) | 🟢 31.22% | 2.4976 | 25 ms | 12 | ✓ 1.4MiB saved | ✓ |

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 10 ms | 10 | ✗  overhead | ✓ |
| g07-v6 | 2.0MiB (2000138) | 🔴 100.01% | 8.0006 | 19883 ms | 18991 | ✗  overhead | ✓ |
| g07-v7 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 8 ms | 12 | ✗  overhead | ✓ |
| g07-v8 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 10 ms | 9 | ✗  overhead | ✓ |
| g07-v9 | 2.0MiB (2000009) | 🔴 100.00% | 8.0000 | 13 ms | 11 | ✗  overhead | ✓ |
| gzip | 2.0MiB (2000330) | 🔴 100.02% | 8.0013 | 62 ms | 13 | ✗  overhead | ✓ |
| bzip2 | 2.0MiB (2009059) | 🔴 100.45% | 8.0362 | 168 ms | 107 | ✗  overhead | ✓ |
| xz | 2.0MiB (2000160) | 🔴 100.01% | 8.0006 | 309 ms | 7 | ✗  overhead | ✓ |
| zstd | 2.0MiB (2000061) | 🔴 100.00% | 8.0002 | 13 ms | 9 | ✗  overhead | ✓ |

### File: E

**Original Size**: 1012112 bytes (989KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 851KiB (870722) | 🔴 86.03% | 6.8824 | 66 ms | 155 | ✓ 139KiB saved | ✓ |
| g07-v6 | 964KiB (986409) | 🔴 97.46% | 7.7968 | 7824 ms | 7309 | ✓ 26KiB saved | ✓ |
| g07-v7 | 872KiB (892886) | 🔴 88.22% | 7.0576 | 11 ms | 14 | ✓ 117KiB saved | ✓ |
| g07-v8 | 989KiB (1012121) | 🔴 100.00% | 8.0001 | 9 ms | 9 | ✗  overhead | ✓ |
| g07-v9 | 847KiB (866359) | 🔴 85.60% | 6.8479 | 662 ms | 177 | ✓ 143KiB saved | ✓ |
| gzip | 877KiB (897332) | 🔴 88.66% | 7.0927 | 47 ms | 11 | ✓ 113KiB saved | ✓ |
| bzip2 | 875KiB (895061) | 🔴 88.43% | 7.0748 | 89 ms | 57 | ✓ 115KiB saved | ✓ |
| xz | 849KiB (868756) | 🔴 85.84% | 6.8669 | 168 ms | 58 | ✓ 140KiB saved | ✓ |
| zstd | 875KiB (895471) | 🔴 88.48% | 7.0780 | 9 ms | 9 | ✓ 114KiB saved | ✓ |

### File: F

**Original Size**: 2097152 bytes (2.0MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 1.7MiB (1728560) | 🔴 82.42% | 6.5939 | 47 ms | 78 | ✓ 360KiB saved | ✓ |
| g07-v6 | 1.7MiB (1763443) | 🔴 84.09% | 6.7270 | 17452 ms | 16777 | ✓ 326KiB saved | ✓ |
| g07-v7 | 1.8MiB (1838063) | 🔴 87.65% | 7.0117 | 14 ms | 18 | ✓ 254KiB saved | ✓ |
| g07-v8 | 2.1MiB (2097161) | 🔴 100.00% | 8.0000 | 13 ms | 10 | ✗  overhead | ✓ |
| g07-v9 | 1.7MiB (1713386) | 🔴 81.70% | 6.5360 | 1048 ms | 226 | ✓ 375KiB saved | ✓ |
| gzip | 1.8MiB (1845625) | 🔴 88.01% | 7.0405 | 77 ms | 21 | ✓ 246KiB saved | ✓ |
| bzip2 | 1.7MiB (1699879) | 🔴 81.06% | 6.4845 | 153 ms | 103 | ✓ 388KiB saved | ✓ |
| xz | 1.7MiB (1727568) | 🔴 82.38% | 6.5901 | 362 ms | 102 | ✓ 361KiB saved | ✓ |
| zstd | 1.8MiB (1840935) | 🔴 87.78% | 7.0226 | 15 ms | 12 | ✓ 251KiB saved | ✓ |

### File: G

**Original Size**: 2526752 bytes (2.5MiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 712KiB (728184) | 🟢 28.82% | 2.3055 | 93 ms | 52 | ✓ 1.8MiB saved | ✓ |
| g07-v6 | 848KiB (867615) | 🟢 34.34% | 2.7470 | 9016 ms | 8395 | ✓ 1.6MiB saved | ✓ |
| g07-v7 | 968KiB (990555) | 🟢 39.20% | 3.1362 | 39 ms | 20 | ✓ 1.5MiB saved | ✓ |
| g07-v8 | 1.8MiB (1838351) | 🟡 72.76% | 5.8204 | 13 ms | 11 | ✓ 673KiB saved | ✓ |
| g07-v9 | 703KiB (719531) | 🟢 28.48% | 2.2781 | 386 ms | 41 | ✓ 1.8MiB saved | ✓ |
| gzip | 913KiB (934659) | 🟢 36.99% | 2.9592 | 236 ms | 21 | ✓ 1.6MiB saved | ✓ |
| bzip2 | 709KiB (725142) | 🟢 28.70% | 2.2959 | 164 ms | 85 | ✓ 1.8MiB saved | ✓ |
| xz | 734KiB (750600) | 🟢 29.71% | 2.3765 | 894 ms | 36 | ✓ 1.7MiB saved | ✓ |
| zstd | 884KiB (905085) | 🟢 35.82% | 2.8656 | 22 ms | 7 | ✓ 1.6MiB saved | ✓ |

### File: H

**Original Size**: 1022760 bytes (999KiB)

| Tool | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved | Lossless |
|------|-----------------|-------|-------------|---------------|-----------------|-------------|----------|
| g07-v5 | 438KiB (447874) | 🟢 43.79% | 3.5033 | 114 ms | 100 | ✓ 562KiB saved | ✓ |
| g07-v6 | 471KiB (481460) | 🟢 47.07% | 3.7660 | 4018 ms | 3763 | ✓ 529KiB saved | ✓ |
| g07-v7 | 471KiB (481597) | 🟢 47.09% | 3.7670 | 33 ms | 26 | ✓ 529KiB saved | ✓ |
| g07-v8 | 580KiB (593367) | 🟡 58.02% | 4.6413 | 12 ms | 11 | ✓ 420KiB saved | ✓ |
| g07-v9 | 436KiB (446377) | 🟢 43.64% | 3.4915 | 636 ms | 61 | ✓ 563KiB saved | ✓ |
| gzip | 440KiB (450169) | 🟢 44.02% | 3.5212 | 51 ms | 12 | ✓ 560KiB saved | ✓ |
| bzip2 | 423KiB (432998) | 🟢 42.34% | 3.3869 | 73 ms | 46 | ✓ 576KiB saved | ✓ |
| xz | 359KiB (366908) | 🟢 35.87% | 2.8699 | 216 ms | 27 | ✓ 641KiB saved | ✓ |
| zstd | 450KiB (459851) | 🟢 44.96% | 3.5969 | 16 ms | 9 | ✓ 550KiB saved | ✓ |

---

## Performance Metrics

### Compression Speed (Average)

| Tool | Avg Compress Time | Ranking |
|------|-------------------|---------|
| g07-v8 | 13 ms | 🥇 #1 |
| zstd | 16 ms | 🥈 #2 |
| g07-v7 | 23 ms | 🥉 #3 |
| g07-v5 | 74 ms |  #4 |
| gzip | 79 ms |  #5 |
| bzip2 | 118 ms |  #6 |
| xz | 374 ms |  #7 |
| g07-v9 | 543 ms |  #8 |
| g07-v6 | 8943 ms |  #9 |

### Decompression Speed (Average)

| Tool | Avg Decompress Time | Ranking |
|------|---------------------|---------|
| zstd | 9 ms | 🥇 #1 |
| g07-v8 | 11 ms | 🥈 #2 |
| gzip | 15 ms | 🥉 #3 |
| g07-v7 | 20 ms |  #4 |
| xz | 40 ms |  #5 |
| bzip2 | 70 ms |  #6 |
| g07-v5 | 75 ms |  #7 |
| g07-v9 | 91 ms |  #8 |
| g07-v6 | 8443 ms |  #9 |

---

## Rankings

### Overall Compression Ratio Ranking

| Rank | Tool | Average Ratio | Files Won | Total Compressed | Performance |
|------|------|---------------|-----------|------------------|-------------|
| 🥇 1 | **xz** | 54.16% | 2 | 6.8MiB | ⭐⭐⭐ |
| 🥈 2 | **g07-v9** | 54.40% | 3 | 6.9MiB | ⭐⭐ |
| 🥉 3 | **g07-v5** | 54.77% | 2 | 6.9MiB | ⭐ |
|  4 | **bzip2** | 54.88% | 1 | 6.9MiB |  |
|  5 | **g07-v6** | 58.41% | 0 | 7.4MiB |  |
|  6 | **zstd** | 58.59% | 0 | 7.4MiB |  |
|  7 | **gzip** | 59.47% | 0 | 7.5MiB |  |
|  8 | **g07-v7** | 59.52% | 0 | 7.5MiB |  |
|  9 | **g07-v8** | 76.42% | 0 | 9.6MiB |  |

### Files Won by Each Tool

- **g07-v5**: 2 file(s)
- **g07-v6**: 0 file(s)
- **g07-v7**: 0 file(s)
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
| g07-v6 | 13MiB | 7.4MiB | 58.41% | 4.6726 | 8 |
| g07-v7 | 13MiB | 7.5MiB | 59.52% | 4.7612 | 8 |
| g07-v8 | 13MiB | 9.6MiB | 76.42% | 6.1136 | 8 |
| g07-v9 | 13MiB | 6.9MiB | 54.40% | 4.3516 | 8 |
| gzip | 13MiB | 7.5MiB | 59.47% | 4.7579 | 8 |
| bzip2 | 13MiB | 6.9MiB | 54.88% | 4.3905 | 8 |
| xz | 13MiB | 6.8MiB | 54.16% | 4.3324 | 8 |
| zstd | 13MiB | 7.4MiB | 58.59% | 4.6872 | 8 |

---

*Report generated on Mon Apr 13 10:59:35 AM WEST 2026*
