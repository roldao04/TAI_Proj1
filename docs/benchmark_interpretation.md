# Benchmark Interpretation Guide

**TAI Project #1 - Group 07**
**For Users and Evaluators**

---

## Purpose

This guide helps you understand our benchmark results, interpret the numbers, and make informed decisions about when to use G07 compressor vs other tools.

---

## Table of Contents

1. [Quick Start: Reading Benchmark Results](#quick-start-reading-benchmark-results)
2. [Understanding Metrics](#understanding-metrics)
3. [When to Use G07](#when-to-use-g07)
4. [Running Your Own Benchmarks](#running-your-own-benchmarks)
5. [Common Questions](#common-questions)

---

## Quick Start: Reading Benchmark Results

### Example Benchmark Output

```
| File | Size | G07 | gzip | bzip2 | xz | zstd | Winner |
|------|------|-----|------|-------|----|----|--------|
| A    | 1.3MiB | 51.85% ⭐ | 58.49% | 54.05% | 53.34% | 53.52% | g07 |
```

**How to read this**:
- **Lower percentage = better compression**
- `51.85%` means compressed file is 51.85% of original size
- `⭐` indicates the winner (best compression for that file)
- G07 won this file, compressing it to 51.85% vs gzip's 58.49%

### Quick Visual Guide

```
🟢 EXCELLENT: < 30%   - Saved 70%+ of space
🟡 GOOD:      30-60%  - Saved 40-70% of space
🔴 POOR:      60-90%  - Saved 10-40% of space
⚫ FAIL:      > 100%  - File got larger!
```

---

## Understanding Metrics

### 1. Compression Ratio (%)

**Definition**: `(compressed_size / original_size) × 100`

**Examples**:
- 50% = file is now half the original size
- 100% = no compression (file same size)
- 102% = file got larger (compression failed)

**What's considered good**:
- Text files: 30-60% (saving 40-70%)
- Binary files: 70-90% (saving 10-30%)
- Random data: ~100% (can't compress)

### 2. Bits per Symbol

**Definition**: `(compressed_size_bits / total_symbols)`

**Interpretation**:
- Perfect compression: 0 bits/symbol (impossible!)
- No compression: 8.0 bits/symbol (1 byte each)
- Typical good compression: 2-5 bits/symbol

**Example**:
- 4.15 bits/symbol = 51.85% ratio (same information, different units)
- Formula: `bits_per_symbol / 8 = ratio`

### 3. Speed Metrics

**Compress Time**: How long it took to compress
- **Very Fast**: < 50ms for 1-2MB files (zstd, gzip)
- **Fast**: 50-200ms (bzip2)
- **Medium**: 200-500ms (xz)
- **Slow**: 500-2000ms (g07 with Order-0)
- **Very Slow**: > 2000ms (g07 with Order-1)

**Decompress Time**: How long to decompress
- Usually faster than compression
- Important for files you'll decompress often

**Speed/Compression Trade-off**:
```
Fast           ←→           Better Compression
zstd → gzip → bzip2 → xz
         ↓
        g07 (very slow, medium compression)
```

### 4. Lossless Verification

**All our tests show "✓ YES"**:
- Means: `decompressed_file == original_file` (byte-for-byte)
- If this ever shows "NO", there's a critical bug
- Lossless guarantee is non-negotiable

---

## When to Use G07

### ✅ USE G07 When...

#### 1. You Have Low-Entropy Text Files
**Examples**: Plain text, source code, logs, configuration files

**Why**: Our Order-1 model excels at capturing local patterns in text.

**Expected performance**:
- 5-10% better than gzip
- Competitive with bzip2
- Possibly better than xz

**Speed**: Acceptable (< 500ms for 1-2MB files)

#### 2. Compression Ratio is Critical
**Scenario**: Archival, bandwidth-limited transmission, long-term storage

**Why**: You're willing to wait for best compression.

**Trade-off**: Be patient with Order-1 (can take seconds)

#### 3. You Have Small Files (< 1MB)
**Why**: Speed penalty is tolerable in absolute terms (< 1 second)

#### 4. Testing/Academic Work
**Why**: G07 demonstrates advanced modeling techniques (Order-1, PPM, adaptive frequencies)

### ❌ DON'T Use G07 When...

#### 1. You Have Structured Data (XML, JSON, CSV)
**Problem**: G07 uses statistical modeling, not dictionary compression

**Alternative**: Use `xz` or `bzip2`
- xz: Best compression on structured data
- bzip2: Good balance, faster than xz

**Performance loss**: G07 can be 2-3x worse on these files

#### 2. Speed is Important
**Problem**: G07 is much slower than alternatives (especially Order-1)

**Alternatives**:
- Need fastest: `zstd` (118x faster than G07)
- Need fast + good: `gzip` (21x faster than G07)

#### 3. You Have Large Files (> 2MB) with Order-1
**Problem**: Can take 3-5+ seconds per file

**Tip**: Use `--model order0` or `--model auto` to avoid this

#### 4. File is Already Compressed
**Examples**: .jpg, .mp4, .zip, .pdf

**Why**: Can't compress random-looking data

**Good news**: G07's auto-selection detects this and stores uncompressed (minimal overhead)

---

## Running Your Own Benchmarks

### Quick Comparison (Single File)

```bash
./benchmarks/compare.sh <your_file>
```

**Output**: Compares G07 vs gzip/bzip2/xz/zstd on your file

### Full Benchmark Suite

```bash
# Compare all models (Order-0 vs Order-1 vs Auto)
./benchmarks/benchmark_models.sh

# Compare versions (v1.0 vs v2.0)
./benchmarks/benchmark_versions.sh

# Compare against all industry tools
./benchmarks/benchmark_all.sh
```

**Results**: Saved in `benchmarks/` as CSV and Markdown files

### Benchmark Your Own Files

```bash
# 1. Put your files in data/ directory
cp /path/to/your/files/* data/

# 2. Run benchmark
./benchmarks/benchmark_all.sh

# 3. View results
cat benchmarks/results.md
```

### Interpret Your Results

**Questions to ask**:

1. **Did G07 win on any files?**
   - YES → Probably text or low-entropy data
   - NO → Likely structured or high-entropy data

2. **What was the compression ratio?**
   - < 60% → Good compression achieved
   - 60-90% → Moderate compression
   - > 100% → File is incompressible (should see "UNCOMPRESSED" model)

3. **How long did it take?**
   - < 500ms → Acceptable
   - 500ms-2s → Slow but tolerable
   - > 2s → Consider using Order-0 or different tool

4. **Which model was selected?**
   - Order-0 → File has high entropy or is small
   - Order-1 → File has patterns Order-1 can exploit
   - Uncompressed → File is incompressible (entropy > 7.5)

---

## Common Questions

### Q: Why did G07 lose on my XML file?

**A**: G07 uses statistical modeling (predicts next byte based on previous bytes). XML has repeated multi-byte structures like `<tag>...</tag>` that dictionary compressors (xz, bzip2) handle much better.

**Solution**: Use `xz` for XML, JSON, CSV, and similar structured formats.

---

### Q: Why is Order-1 so slow on my file?

**A**: Order-1 maintains 256 separate frequency tables (one per context). For files with complex patterns, this causes:
- Frequent context switches
- Many escapes (unseen symbols)
- Constant table updates
- Poor cache locality

**Solution**:
- Use `--model order0` for faster compression
- Use `--model auto` to let G07 decide (it avoids Order-1 on high-entropy files)

---

### Q: What does "TIMEOUT" mean in benchmark results?

**A**: Compression or decompression took > 60 seconds and was killed by the timeout protection.

**Common causes**:
- Order-1 on very high-entropy files
- Very large files
- Complex patterns causing worst-case behavior

**Solution**: Benchmarks now use `--yes` flag to avoid interactive prompts that cause timeouts.

---

### Q: Which metric is most important?

**A**: Depends on your use case:

| Use Case | Priority Metric | Why |
|----------|----------------|-----|
| Archival | Compression Ratio | Space is expensive, time is free |
| Real-time | Compress Speed | Users are waiting |
| Downloads | Decompress Speed | Bandwidth cheap, CPU cheap, user waiting |
| Backup | Both speeds | Runs frequently, storage expensive |
| Analysis | Lossless ✓ | Must be perfect |

**For G07 evaluation**: Focus on **compression ratio** and **where we win/lose**.

---

### Q: How do I know if a file is "compressible"?

**Method 1 - Quick Test**:
```bash
./bin/compress myfile.dat /tmp/test.g07
# Look at the ratio in output
```

**Method 2 - Calculate Entropy**:
```bash
# If entropy is displayed during compression:
# - Entropy < 6.0 → Very compressible
# - Entropy 6-7 → Moderately compressible
# - Entropy > 7.5 → Barely compressible
```

**Method 3 - File Type**:
- Text, source code → Usually compressible
- Images (PNG, BMP) → Sometimes compressible
- JPEG, MP4, ZIP → Already compressed, won't compress further

---

### Q: What's the best all-around compressor?

**From our benchmarks**:

1. **xz**: Best compression (54.16% avg), won 4/8 files
2. **bzip2**: Good compression (54.88% avg), faster than xz
3. **zstd**: Best speed (11ms avg), decent compression (58.59%)
4. **gzip**: Fast (63ms), acceptable compression (59.47%)
5. **G07**: Wins on text, loses on structured, very slow

**Recommendation**: For general use, **xz** or **zstd** depending on speed requirements.

**G07's niche**: Low-entropy text where you want to beat all competitors.

---

### Q: Is G07 suitable for production use?

**Honest answer**: Not yet.

**Reasons**:
- Too slow for most applications
- Loses badly on structured data
- Not battle-tested at scale
- No streaming mode

**However**:
- Excellent for academic/research use
- Demonstrates advanced modeling
- Best-in-class on specific file types
- Could be optimized for speed (future work)

**Production-ready alternatives**: zstd, xz, bzip2, gzip

---

##Appendix: Model Selection Guide

### Auto-Selection Algorithm

G07's `--model auto` uses these rules:

```
1. Calculate entropy from first 8KB sample
2. Get file size

3. IF entropy > 7.5:
     → Use UNCOMPRESSED (can't compress random data)

4. ELSE IF entropy > 6.8:
     → Use ORDER-0 (Order-1 too slow for marginal gain)

5. ELSE IF file_size < 100KB:
     → Use ORDER-0 (adaptive overhead not worth it)

6. ELSE:
     → Use ORDER-1 (low entropy, good patterns expected)
```

**Success rate**: 100% on our test files (8/8 correct choices)

### Manual Model Selection

**Force Order-0**:
```bash
./bin/compress input.txt output.g07 --model order0
```
**When**: Need speed, file is high-entropy, or Order-1 timed out

**Force Order-1**:
```bash
./bin/compress input.txt output.g07 --model order1
```
**When**: You know file is low-entropy text and want best compression

**Use Auto** (recommended):
```bash
./bin/compress input.txt output.g07 --model auto
# or just:
./bin/compress input.txt output.g07
```

---

## Summary

**Key Takeaways**:
1. **Lower compression ratio = better** (50% is better than 60%)
2. **G07 wins on text** (Order-1 model excels here)
3. **G07 loses on structured data** (dictionary methods beat statistical)
4. **G07 is slow** (especially Order-1: 28-90x slower than Order-0)
5. **Auto-selection works well** (90%+ success rate)
6. **Speed vs compression trade-off** is fundamental to compression

**Bottom line**: G07 is a research-grade compressor with excellent text performance but limited general-purpose utility. Use xz/zstd for production needs.

---

*Guide version: 1.0*
*Last updated: March 6, 2026*
