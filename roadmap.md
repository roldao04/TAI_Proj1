# Maximum Compression Roadmap - Phase-Based Plan

## Current State
- **v5.0 (Best):** 54.73% avg compression ratio (beats bzip2 by 0.20pp)
- **v6.1 (Failed):** 59.85% avg (static mixing doesn't work)
- **Target:** 40-50% avg (PAQ-level compression)

---

## Phase 1: Adaptive Mixing Foundation
**Goal:** Fix v6.1's static mixing flaw → validate adaptive approach
**Expected Outcome:** 56-58% avg (proves adaptive concept works)
**Risk Level:** Low ⚠️

### What to Build:
1. **Remove static training from v6.1**
   - Delete `train_weights()` function in `src/model/context_mixer.cpp`
   - Initialize mixer with equal weights: `{1.0, 1.0, 1.0, 1.0, 1.0}`

2. **Implement adaptive weight updates**
   ```cpp
   // After encoding each byte (both encoder & decoder):
   for (int i = 0; i < num_orders; i++) {
       error = actual_byte - predictions[i].probability;
       gradient = stretched_predictions[i] * error;
       weights[i] += learning_rate × gradient × confidence[i];
   }
   ```
   - No header storage needed (both sides start equal, update identically)
   - Learning rate: start with 0.002 (same as v6.1)

3. **Testing**
   - Verify lossless roundtrip on all 8 files
   - Benchmark: should beat v6.1 static (59.85%)
   - If adaptive ≤ 58% → success, proceed to Phase 2

### Files to Modify:
- `src/model/context_mixer.cpp` - Remove training, add adaptive updates
- `src/compressor_v6.cpp` - Remove training call
- `include/model/context_mixer.h` - Update method signatures

---

## Phase 2: Bit-Level Architecture ⭐ **CRITICAL PHASE**
**Goal:** Move from byte-level to bit-level prediction
**Expected Outcome:** 48-52% avg (6-10pp improvement over v5.0)
**Risk Level:** Medium 🔥

### What to Build:

#### Part A: Bit-Level Arithmetic Coder
- Modify range coder to encode **one bit at a time**
- Update range after each bit based on probability
- Use existing `bit_arithmetic_coder.cpp` as starting point

```cpp
void encode_bit(int bit, double probability) {
    uint64_t range_size = high - low;
    uint64_t mid = low + (range_size * probability);

    if (bit == 0) {
        high = mid;
    } else {
        low = mid + 1;
    }

    // Renormalization...
}
```

#### Part B: Bit-Context PPM
- **Context = last N bits** (not bytes)
- **Orders:** {4, 8, 12, 16, 24} bits
  - 4 bits = 0.5 bytes context
  - 24 bits = 3 bytes context
- **Hash table:** Reuse v6.1's 8M context structure
- **Prediction:** Each order predicts probability of next bit = 1

```cpp
class BitLevelPPM {
    std::unordered_map<uint64_t, BitContext> contexts;
    std::vector<int> bit_history;  // Last 256 bits

    double predict_next_bit(int order);
    void update(int bit, int order);
};
```

#### Part C: Logistic Mixing
- Better than linear mixing for bit-level prediction

```cpp
double mix_predictions(vector<double>& probs, vector<double>& weights) {
    double mixed_logit = 0;
    double sum_weights = 0;

    for (int i = 0; i < num_orders; i++) {
        double logit = log(probs[i] / (1 - probs[i]));  // Stretch
        mixed_logit += weights[i] * logit;
        sum_weights += weights[i];
    }

    mixed_logit /= sum_weights;
    return 1.0 / (1.0 + exp(-mixed_logit));  // Squash
}
```

#### Part D: Integration
- Pipeline: `BWT → MTF → ZRLE → Bit-Level PPM → Mixer → Bit Arithmetic Coder`
- Each byte generates 8 bit predictions
- Adaptive weights update after each bit

### Files to Create/Modify:
- `include/model/bit_ppm.h` - New bit-level PPM
- `src/model/bit_ppm.cpp` - Implementation
- `src/arithmetic/bit_arithmetic_coder.cpp` - Enhance existing
- `src/compressor_v7.cpp` - New compressor pipeline
- `src/decompressor_v7.cpp` - New decompressor

### Testing Strategy:
1. Start with Order-4 only → verify roundtrip works
2. Add Order-8 → benchmark
3. Add remaining orders → full benchmark
4. Target: < 52% on Files A, B, C by end of phase

---

## Phase 3: Advanced Models (Beyond PPM)
**Goal:** Add specialized models for different pattern types
**Expected Outcome:** 42-48% avg (approaching PAQ)
**Risk Level:** High 🔥🔥

### Model 1: Match Model (Highest Impact)
**Purpose:** Predict bits from exact byte sequence matches (LZ77-style)

```cpp
class MatchModel {
    // Search last 2-8MB for longest exact match
    struct Match {
        int position;      // Where match starts
        int length;        // How many bytes match
        int confidence;    // How reliable (longer = better)
    };

    double predict_next_bit(int bit_position_in_byte) {
        Match m = find_longest_match();
        if (m.length > 8) {
            // Strong match - predict from matched sequence
            uint8_t next_byte = buffer[m.position + m.length];
            int bit = (next_byte >> bit_position_in_byte) & 1;
            return bit ? 0.95 : 0.05;  // High confidence
        }
        return 0.5;  // No match
    }
};
```

**Best for:** Files C, G (structured binary with repeated blocks)

### Model 2: Sparse Model (Long-Range Patterns)
**Purpose:** Capture periodic patterns (headers every N bytes)

```cpp
class SparseModel {
    // Context = order-1 + byte at position -N
    std::vector<int> gaps = {1, 2, 4, 8, 16, 32, 64, 128, 256};

    uint64_t compute_sparse_context(int gap) {
        uint8_t recent = history[pos - 1];
        uint8_t distant = history[pos - gap];
        return (uint64_t)recent << 8 | distant;
    }
};
```

**Best for:** Files A, B, C (structured data with regular patterns)

### Model 3: Word Model (Dictionary)
**Purpose:** Predict common byte sequences

```cpp
class WordModel {
    std::map<uint32_t, std::string> dictionary;  // Hash → word

    // Build dictionary from input (first pass)
    void build_dictionary() {
        // Find frequently repeated 4-16 byte sequences
        // Store top 10000 sequences
    }

    double predict_next_bit(int bit_pos) {
        // If currently in a dictionary word, predict continuation
        auto word = find_current_word();
        if (word != end() && position_in_word < word.length) {
            // Know next byte from dictionary
            uint8_t next = word[position_in_word];
            return extract_bit(next, bit_pos);
        }
        return 0.5;
    }
};
```

**Best for:** File A (English text with repeated words)

### Model 4: Additional PPM Orders
- Add orders: {2, 6, 20, 32, 48} bits
- Total: 10 PPM orders + 3 specialized models = 13 models in mixer

### Model 5: SSE (Secondary Symbol Estimation)
**Purpose:** Correct bias in mixed probability

```cpp
class SSE {
    // Context: last 8 (prediction, actual_bit, error) tuples
    std::map<SSEContext, double> adjustments;

    double correct_probability(double mixed_prob, SSEContext ctx) {
        double adjustment = adjustments[ctx];
        return clamp(mixed_prob + adjustment, 0.01, 0.99);
    }
};
```

**Gain:** +0.5-1pp improvement

### Integration:
- Context mixer now handles 13 models
- Each model returns (probability, confidence)
- Adaptive weights for all 13 models
- Update after each bit

### Files to Create:
- `include/model/match_model.h` + `.cpp`
- `include/model/sparse_model.h` + `.cpp`
- `include/model/word_model.h` + `.cpp`
- `include/model/sse_model.h` + `.cpp`
- Modify `src/compressor_v7.cpp` → `src/compressor_v8.cpp`

---

## Phase 4: Optimization & Polish
**Goal:** Squeeze last 1-2pp, ensure < 8GB RAM
**Expected Outcome:** 40-45% avg (final submission quality)
**Risk Level:** Low ⚠️

### Optimization Tasks:

#### 1. Memory Management
- **Current:** ~6-7GB (10 PPM orders + match buffer + hash tables)
- **Target:** < 8GB
- **Actions:**
  - Reduce hash table to 32M contexts if needed
  - Limit match model search to 4MB window
  - Use smaller data types (uint16_t counts instead of uint32_t)

#### 2. Speed Optimization (Optional)
- Profile with `perf` or `gprof`
- Identify hot paths (likely: bit encoding, hash lookups)
- Add lookup tables for logit/squash functions
- Inline critical functions
- SIMD for hash computation (AVX2)

#### 3. Preprocessing Tuning
- **BWT block size:** Test {1MB, 2MB, 3MB, 4MB}
  - Larger blocks = better compression but more RAM
- **ZRLE:** Fix rank-255 issue or disable selectively
- **MTF variants:** Test standard MTF vs MTF-2 (move-to-2nd)

#### 4. Per-File Model Selection
```cpp
if (file_entropy > 7.5) {
    // File D (random) - store uncompressed
    store_raw();
} else if (file_entropy > 6.8) {
    // Files E, F (high entropy) - use fewer models
    enable_models({PPM_4, PPM_8, PPM_12, MATCH});
} else {
    // Files A, B, C, G, H (low entropy) - use all models
    enable_all_models();
}
```

#### 5. Documentation
- Write comprehensive README for v8.0
- Document architecture (diagrams)
- Create comparison table (v5.0 vs v6.1 vs v7.0 vs v8.0)
- Benchmark charts

### Files to Create:
- `versions/g07_v8.0/README.md`
- `versions/g07_v8.0/ARCHITECTURE.md`
- `benchmarks/results_v8.csv`

---

## Decision Points & Success Criteria

### After Phase 1:
- ✅ **Success:** Adaptive mixing works, ratio ≤ 58%
- ⚠️ **Partial:** Works but ratio > 58% → debug, then proceed
- ❌ **Failure:** Broken decompression → fix or skip to Phase 2

### After Phase 2: **CRITICAL CHECKPOINT**
- ✅ **Excellent:** Ratio < 50% → proceed immediately to Phase 3
- ✅ **Good:** Ratio 50-52% → optimize for 1 cycle, then Phase 3
- ⚠️ **Marginal:** Ratio 52-54% → still beats v5.0, but investigate issues
- ❌ **Failure:** Ratio > 54% → major bug, debug or revert

### After Phase 3:
- ✅ **Amazing:** Ratio < 45% → proceed to polish
- ✅ **Great:** Ratio 45-48% → proceed to polish
- ⚠️ **Acceptable:** Ratio 48-52% → still good, but debug
- ❌ **Problem:** Ratio > 52% → models not helping, investigate

### After Phase 4:
- 🎯 **Target Met:** Ratio 40-48% avg → SUBMIT
- ⚠️ **Close:** Ratio 48-52% → acceptable, document limitations
- ❌ **Missed:** Ratio > 52% → fall back to best working phase

---

## Expected Results by Phase

| Phase | Avg Ratio | vs v5.0 | vs xz (54.16%) | Speed | Status |
|-------|-----------|---------|----------------|-------|--------|
| v5.0 (baseline) | 54.73% | — | +0.57pp | 25 MB/s | ✅ Current |
| Phase 1 (Adaptive) | 56-58% | −2pp | +2pp worse | 100 KB/s | Foundation |
| Phase 2 (Bit-level) | 48-52% | +4-6pp | +6pp better | 10-20 KB/s | ⭐ Main Goal |
| Phase 3 (Advanced) | 42-48% | +8-12pp | +12pp better | 1-5 KB/s | Stretch Goal |
| Phase 4 (Polished) | 40-45% | +10-14pp | +14pp better | 1-5 KB/s | 🎯 Target |

---

## Fallback Strategy

If Phase 2 or 3 fails catastrophically, pivot to **v5.0 Incremental Improvements:**

### Quick Wins:
1. **Order-2 PPM** (upgrade from Order-1)
   - Context: last 2 bytes
   - Escape chain: Order-2 → Order-1 → Order-0
   - Expected: ~1pp improvement

2. **Fix ZRLE rank-255 issue**
   - Use symbol-level RLE instead of byte-level
   - Improves Files G, H
   - Expected: ~0.5pp improvement

3. **Better block sizes**
   - Test 1500KB, 2500KB, 3000KB
   - Expected: ~0.5pp improvement

**Fallback Result:** 52-53% avg (still beats xz)

---

## Key Files Overview

### Phase 1 Files:
- `src/model/context_mixer.cpp` - Adaptive updates
- `src/compressor_v6.cpp` - Remove training

### Phase 2 Files:
- `include/model/bit_ppm.h/cpp` - NEW
- `src/arithmetic/bit_arithmetic_coder.cpp` - MODIFY
- `src/compressor_v7.cpp` - NEW
- `src/decompressor_v7.cpp` - NEW

### Phase 3 Files:
- `include/model/match_model.h/cpp` - NEW
- `include/model/sparse_model.h/cpp` - NEW
- `include/model/word_model.h/cpp` - NEW
- `include/model/sse_model.h/cpp` - NEW
- `src/compressor_v8.cpp` - NEW

### Phase 4 Files:
- `versions/g07_v8.0/README.md` - NEW
- `versions/g07_v8.0/ARCHITECTURE.md` - NEW
- `benchmarks/results_v8.csv` - NEW

---

## Success Metrics

- **Phase 1 Success:** Adaptive works (even if worse than v5.0)
- **Phase 2 Success:** < 52% avg ⭐ **CRITICAL MILESTONE**
- **Phase 3 Success:** < 48% avg
- **Phase 4 Success:** 40-48% avg, < 8GB RAM, full documentation

**Ultimate Goal:** Beat PAQ-lite compression while staying under 8GB memory limit.
