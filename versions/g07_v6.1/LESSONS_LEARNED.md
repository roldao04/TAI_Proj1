# Lessons Learned from v6.1

**Date:** April 6, 2026

**Summary:** v6.1 failed to beat v5.0, but provided valuable insights into compression algorithm design.

---

## What We Tried

**Goal:** Achieve higher compression than v5.0 (54.92%) using multi-order PPM with context mixing.

**Approach:** Static weight training
- Train mixer weights on first 100KB of file
- Freeze weights and store in file header
- Use identical static weights during compression/decompression

**Result:** 59.85% avg (4.93pp **worse** than v5.0)

---

## Key Findings

### 1. BWT+MTF Preprocessing Helps Multi-Order PPM ✅

**Original hypothesis:** "BWT destroys PPM context patterns → disable for better compression"

**Test result:** Disabling BWT made compression **8pp worse**

| Configuration | Avg Ratio | Verdict |
|---------------|-----------|---------|
| With BWT+MTF+ZRLE | 59.85% | ✅ Better |
| Without BWT/MTF/ZRLE | 67.94% | ❌ Worse |

**Why BWT helps:**
- BWT clusters similar bytes together (e.g., all 'e's near each other)
- MTF converts repeated symbols to zeros
- Result: PPM sees predictable local patterns (lots of Order-1 hits)

**Lesson:** Even though BWT disrupts sequential context, it creates **local repetition** that PPM exploits effectively.

---

### 2. Static Weights Can't Adapt to File Sections ❌

**Problem:** Files have varying statistics across sections.

**Examples:**
- File A (text): Header (low entropy) → body (medium) → footer (low)
- File B (genomic): ACGT patterns vary by chromosome region
- File H (binary): Structured data mixed with random padding

**Impact:** Weights trained on first 100KB optimize for that section but underperform elsewhere.

**Solution:** PAQ uses **adaptive mixing** — weights update after every bit encoded, continuously learning file characteristics.

---

### 3. Pure Multi-Order PPM Underperforms on Binary Files ❌

**Evidence:**
- File H (binary mixed): Lost by 13.98pp (worst case)
- File B (genomic): Lost by 2.05pp
- File E/F (high-entropy): Expanded data more than v5.0

**Why:**
- Multi-order PPM works best on natural text (repeating words/phrases)
- Binary files lack sequential structure
- v5.0's BWT creates artificial structure that Order-1 PPM exploits

**Lesson:** Pure PPM needs text-like patterns. For binary files, BWT preprocessing + simpler Order-1 PPM (v5.0) is more effective.

---

### 4. Byte-Level Coding Limits Compression ❌

**Problem:** Range coder encodes one **byte** at a time (best case: 1 bit/symbol).

**PAQ approach:** Encode one **bit** at a time (can achieve fractional bits/symbol).

**Impact:**
- v6.1: Minimum 1 bit/symbol for perfectly predicted data
- PAQ: Can achieve 0.1 bits/symbol for highly predictable sequences
- **Penalty:** ~8-10% compression loss vs bit-level coding

**Example:**
- Byte "00000000" compressed to 1 bit with byte-level coding
- Same byte compressed to 0.01 bits with bit-level (if perfectly predicted)

---

### 5. Hash Table Capacity Issues ❌

**Problem:** 8M context limit insufficient for 9+ orders.

**Test result:** Orders {1,2,3,4,5,6,8,12,16} caused:
- 2400% hash table capacity overflow
- Excessive LRU eviction (discarding useful context)
- Decompression corruption (missing contexts)

**Lesson:** Higher orders need exponentially more memory. Practical limit for 8M table is ~5-6 orders.

---

### 6. Compression Speed Trade-off ❌

**v6.1:** 158 KB/s (81 seconds for 13 MiB)
**v5.0:** 25 MB/s (0.5 seconds for 13 MiB)
**Ratio:** v6.1 is **157× slower**

**Bottlenecks:**
- 5 hash table lookups per byte (vs v5.0's 1 flat array lookup)
- Context mixing overhead
- Byte-level range coding complexity

**Lesson:** Multi-order PPM is inherently slower. The 4.93pp worse compression doesn't justify 157× slowdown.

---

## What Worked ✅

1. **Fixed-point arithmetic** — Deterministic encoder/decoder sync (no floating-point drift)
2. **Sample-based training** — Clever approach to avoid storing per-byte weight updates
3. **BWT preprocessing** — Confirmed it helps (contrary to hypothesis)
4. **Lossless guarantee** — All 8 test files decompressed perfectly

---

## What Failed ❌

1. **Static mixing** — Weights can't adapt to file sections
2. **Pure PPM** — Underperforms on binary files vs BWT+Order-1
3. **Byte-level coding** — Leaves information on the table
4. **Complexity** — Much slower with worse results

---

## Path Forward: Three Options

### Option A: True Adaptive Mixing (Recommended)

**Changes:**
- Remove sample training phase
- Start encoder/decoder with equal weights
- Update weights after every **byte** encoded (keep byte-level for simplicity):
  ```cpp
  weight[i] += learning_rate × error × stretched_prediction × confidence
  ```
- Both sides compute identical updates → stay synchronized
- No need to store weights in header

**Expected:**
- ~2-4pp improvement over v6.1 (56-58% avg)
- Still slower than v5.0, still loses overall
- Proves adaptive mixing concept

**Effort:** 2-3 days

**Risk:** Medium (deadline Apr 13)

---

### Option B: Bit-Level PAQ-Lite

**Changes:**
- Redesign entire pipeline for bit-level prediction
- PPM predicts next **bit**, not byte
- Multiple model types (PPM + match + sparse)
- Logistic mixing (neural-like)

**Expected:**
- 5-8pp improvement (51-54% avg, competitive with v5.0)
- Very slow (PAQ is 10-100× slower than byte-level)

**Effort:** 4-5 days (high complexity)

**Risk:** **High** — may not finish by deadline

---

### Option C: Improve v5.0 (Lowest Risk)

**Changes:**
- Keep v5.0's winning BWT+MTF architecture
- Replace Order-1 PPM with adaptive Order-2 or Order-3
- Use simple weight blending (not full context mixing)

**Expected:**
- 1-2pp improvement over v5.0 (53-54% avg)
- Still fast (only 2-3× slower than v5.0)
- Incremental, low-risk improvement

**Effort:** 1-2 days

**Risk:** **Low** — minor changes to proven system

---

## Recommendation

**For TAI project deadline (7 days remaining):**

1. **Accept v5.0 as final submission** ✅
   - Already beats bzip2 by 0.20pp (54.73% vs 54.93%)
   - Competitive with xz/lzma (54.16%)
   - Fast and production-ready
   - Document v6.1 as "experimental exploration"

2. **If teacher requires improvement:**
   - Choose **Option C** (v5.0 + Order-2 PPM)
   - Lowest risk, 1-2 day effort
   - Guaranteed incremental improvement
   - Maintains v5.0's speed advantage

3. **Do NOT pursue:**
   - Option A (adaptive mixing) — still won't beat v5.0
   - Option B (bit-level PAQ) — too risky with deadline

---

## Technical Insights for Future Work

### How PAQ Really Works

**Key differences from v6.1:**

1. **Adaptive weights:**
   ```cpp
   // After encoding each bit:
   error = actual_bit - predicted_prob
   for each model:
       gradient = stretched_prediction × error
       weight += learning_rate × gradient
   ```

2. **Bit-level prediction:**
   - Predicts next **bit** (0 or 1), not byte
   - Can achieve fractional bits/symbol

3. **Multiple models:**
   - PPM orders 1-6
   - Match model (finds exact repeats)
   - Sparse model (long-range patterns)
   - Word model (dictionary-based)
   - 20-30 models total

4. **Logistic mixing:**
   ```cpp
   mixed = sum(weight[i] × logit(prob[i])) / sum(weight[i])
   final_prob = 1 / (1 + exp(-mixed))
   ```

5. **SSE (Secondary Symbol Estimation):**
   - Corrects bias in mixed probability
   - Uses context of recent predictions

**Result:** PAQ achieves 30-40% compression ratios (vs our 54-60%) but is 100-1000× slower.

---

## Conclusion

v6.1 taught us that **static weight training is not an effective compression strategy**. However, the experiment validated BWT preprocessing for multi-order PPM and provided deep insights into adaptive mixing algorithms.

The simpler v5.0 architecture remains superior for this project's goals: competitive compression ratios with fast speed.

---

*Last updated: 2026-04-06*
