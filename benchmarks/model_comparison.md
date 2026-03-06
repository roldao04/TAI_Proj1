# Model Comparison Benchmark

**Project**: TAI - Algorithmic Information Theory - Group 07
**Date**: $(date +"%Y-%m-%d")
**Comparison**: Order-0 vs Order-1 vs Auto-Selection

---

## Executive Summary

This benchmark compares three model configurations of our compressor:

- **Order-0**: Static frequency model (fast, universal)
- **Order-1**: Adaptive context model using previous byte as context (better compression for low-entropy data)
- **Auto**: Intelligent selection based on file entropy and size

---

## Table of Contents
1. [Quick Summary](#quick-summary)
2. [Detailed File-by-File Analysis](#detailed-file-by-file-analysis)
3. [Model Performance Characteristics](#model-performance-characteristics)
4. [Speed vs Compression Trade-offs](#speed-vs-compression-trade-offs)
5. [Auto-Selection Effectiveness](#auto-selection-effectiveness)
6. [Recommendations](#recommendations)

---

## Quick Summary

### Compression Ratio Comparison

| File | Size | Order-0 | Order-1 | Auto | Best Model | Winner Ratio |
|------|------|---------|---------|------|------------|--------------|
| A | 1.3MiB | 52.31% | **51.85%** ⭐ | 51.85% | order1 | 51.85% |
| B | 1.2MiB | 67.56% | **47.44%** ⭐ | 47.44% | order1 | 47.44% |
| C | 2.0MiB | 66.18% | **45.77%** ⭐ | 45.77% | order1 | 45.77% |
| D | 2.0MiB | 100.10% | TIMEOUT% | **100.00%** ⭐ | auto | 100.00% |
| E | 989KiB | **88.41%** ⭐ | TIMEOUT% | 88.41% | order0 | 88.41% |
| F | 2.0MiB | **88.04%** ⭐ | TIMEOUT% | 88.04% | order0 | 88.04% |
| G | 2.5MiB | 41.92% | **28.81%** ⭐ | 28.81% | order1 | 28.81% |
| H | 999KiB | 80.85% | **60.99%** ⭐ | 60.99% | order1 | 60.99% |

⭐ = Best model for that file

### Files Won by Each Model

- **order0**: 2 file(s)
- **order1**: 5 file(s)
- **auto**: 1 file(s)

---

## Detailed File-by-File Analysis

### File: A

**Original Size**: 1308765 bytes (1.3MiB)

| Model | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved |
|-------|-----------------|-------|-------------|---------------|-----------------|-------------|
| order0 | 669KiB (684554) | 52.31% | 4.1844 | 11 ms | 37 ms | 610KiB |
| order1 | 663KiB (678619) | 51.85% | 4.1481 | 312 ms | 238 ms | 616KiB |
| auto | 663KiB (678619) | 51.85% | 4.1481 | 326 ms | 244 ms | 616KiB |

**Analysis**:
- Order-1 is 0.46pp better (0.9% improvement), but 28.4x slower
- Auto-selection chose **order1** ✓ (optimal choice)

### File: B

**Original Size**: 1168767 bytes (1.2MiB)

| Model | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved |
|-------|-----------------|-------|-------------|---------------|-----------------|-------------|
| order0 | 772KiB (789665) | 67.56% | 5.4051 | 11 ms | 33 ms | 371KiB |
| order1 | 542KiB (554460) | 47.44% | 3.7952 | 746 ms | 781 ms | 600KiB |
| auto | 542KiB (554460) | 47.44% | 3.7952 | 741 ms | 778 ms | 600KiB |

**Analysis**:
- Order-1 is 20.12pp better (29.8% improvement), but 67.8x slower
- Auto-selection chose **order1** ✓ (optimal choice)

### File: C

**Original Size**: 2000000 bytes (2.0MiB)

| Model | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved |
|-------|-----------------|-------|-------------|---------------|-----------------|-------------|
| order0 | 1.3MiB (1323522) | 66.18% | 5.2941 | 18 ms | 48 ms | 661KiB |
| order1 | 895KiB (915495) | 45.77% | 3.6620 | 1255 ms | 1330 ms | 1.1MiB |
| auto | 895KiB (915495) | 45.77% | 3.6620 | 1217 ms | 1320 ms | 1.1MiB |

**Analysis**:
- Order-1 is 20.41pp better (30.8% improvement), but 69.7x slower
- Auto-selection chose **order1** ✓ (optimal choice)

### File: D

**Original Size**: 2000000 bytes (2.0MiB)

| Model | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Space Saved |
|-------|-----------------|-------|-------------|---------------|-----------------|-------------|
