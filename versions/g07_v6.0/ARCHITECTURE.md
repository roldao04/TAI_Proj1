# v6.1 Architecture - Static Weight Training Implementation

**Core Concept:** Sample-based static weight training for multi-order PPM with context mixing.

---

## System Overview

```
┌────────────────────────────────────────────────────────────┐
│                    COMPRESSION PHASE                       │
└────────────────────────────────────────────────────────────┘

Input File
    ↓
┌─────────────────────────────────────┐
│ Step 1: Sample-Based Weight Training│
│  • Read first 100KB of input        │
│  • Train PPM + mixer on sample      │
│  • Freeze final weights             │
│  • Result: 32-byte static weights   │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Step 2: Preprocessing (v5.0 reuse) │
│  • BWT (Burrows-Wheeler Transform)  │
│  • MTF (Move-To-Front)              │
│  • ZRLE (Zero-Run Length Encoding)  │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Step 3: Multi-Order PPM Encoding    │
│  • 5 orders: {1, 2, 3, 4, 5}        │
│  • Each order predicts next byte    │
│  • Hash table: 8M contexts          │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Step 4: Context Mixing (Static)     │
│  • Load frozen weights from Step 1  │
│  • Mix 5 predictions → final prob   │
│  • NO weight updates during encode  │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Step 5: Range Coding (byte-level)   │
│  • Encode byte using mixed prob     │
│  • Output compressed bitstream      │
└─────────────────────────────────────┘
    ↓
File Header (32-byte weights) + Compressed Data


┌────────────────────────────────────────────────────────────┐
│                   DECOMPRESSION PHASE                      │
└────────────────────────────────────────────────────────────┘

Compressed File
    ↓
┌─────────────────────────────────────┐
│ Step 1: Read Header                 │
│  • Extract 32-byte static weights   │
│  • Initialize mixer with weights    │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Step 2: Multi-Order PPM Decoding    │
│  • Same 5 orders as encoder         │
│  • Range decode using mixed prob    │
│  • Update PPM statistics            │
│  • NO weight updates (static)       │
└─────────────────────────────────────┘
    ↓
┌─────────────────────────────────────┐
│ Step 3: Reverse Preprocessing       │
│  • Reverse ZRLE                     │
│  • Reverse MTF                      │
│  • Reverse BWT (inverse transform)  │
└─────────────────────────────────────┘
    ↓
Original File (lossless reconstruction)
```

---

## Component Details

### 1. Static Weight Training (Sample-Based)

**Purpose:** Pre-compute optimal mixer weights for the file without storing per-byte updates.

**Algorithm:**
```cpp
1. Read first min(100KB, file_size) bytes as training sample
2. Apply BWT+MTF+ZRLE to sample (same as full pipeline)
3. Initialize PPM models (5 orders) + mixer (equal weights)
4. For each byte in sample:
   a. Get predictions from all 5 orders
   b. Mix predictions using current weights
   c. Update weights via gradient descent:
      update = learning_rate × error × stretched_prediction
      weight[i] += update
5. Freeze final weights (convert to fixed-point)
6. Store 32 bytes (5 orders × 8 bytes/weight) in file header
```

**Key decision:** Training on 100KB balances:
- Enough data to learn file characteristics
- Fast training (~5-10% of total compression time)
- Small header overhead (32 bytes ≈ 0.002% of 1MB file)

**Limitation:** Weights optimal for first 100KB may not represent entire file.

---

### 2. Multi-Order PPM

**Data structure:**
```cpp
class MultiOrderPPM {
    std::vector<int> orders;                    // {1, 2, 3, 4, 5}
    std::unordered_map<uint64_t, ContextEntry> hash_table;  // 8M capacity
    std::vector<uint8_t> history;               // Last 8KB bytes
    size_t history_pos;                         // Circular buffer position
};

struct ContextEntry {
    uint32_t counts[256];    // Symbol frequency counts
    uint32_t total;          // Sum of counts
    uint16_t timestamp;      // LRU tracking
};
```

**Context hashing:**
```cpp
uint64_t hash = order;  // Start with order number
for (int i = 0; i < order; i++) {
    hash = hash * 31 + history[(history_pos - 1 - i) % 8192];
}
```

**Prediction:**
```cpp
For each order {1, 2, 3, 4, 5}:
    1. Compute hash from last 'order' bytes
    2. Look up context in hash table
    3. If context exists:
       probability = count[symbol] / total
       confidence = min(total / 256.0, 1.0)  // More observations = higher confidence
    4. If context not found:
       probability = 0.5 (uniform guess)
       confidence = 0.0 (no information)
```

---

### 3. Context Mixer (Fixed-Point Arithmetic)

**Purpose:** Combine 5 predictions into single probability distribution using pre-trained static weights.

**Fixed-point scale:** `FIXED_POINT_SCALE = 65536` (16-bit fractional precision)

**Mixing algorithm:**
```cpp
// Weights frozen from training phase (stored in header)
vector<int64_t> weights_fixed = {w1, w2, w3, w4, w5};

double mix(vector<Prediction>& predictions) {
    int64_t sum_stretched = 0;
    int64_t sum_weights = 0;

    for (int i = 0; i < 5; i++) {
        if (predictions[i].valid && predictions[i].confidence > 0) {
            // Convert probability to log-odds (stretch)
            int32_t stretched = stretch_table[prob_to_index(predictions[i].probability)];

            // Weight by confidence and static weight
            int64_t effective_weight = weights_fixed[i] * confidence_fixed / SCALE;

            sum_stretched += effective_weight * stretched;
            sum_weights += effective_weight;
        }
    }

    // Average in stretched space
    int32_t mixed_stretched = sum_stretched / sum_weights;

    // Convert back to probability (squash)
    return squash_table[stretched_to_index(mixed_stretched)];
}
```

**Why fixed-point?**
- Ensures encoder and decoder compute **identical** values (no floating-point rounding differences)
- Deterministic across platforms (Intel vs ARM, Linux vs Windows)
- Faster than floating-point on some CPUs

**NO updates during compression:**
```cpp
void update(...) {
    // In v6.1: This function does NOTHING
    // Weights remain frozen from training phase
}
```

---

### 4. Range Coder (Byte-Level)

Reuses v5.0 range coder - encodes one byte at a time using mixed probability distribution.

**Encoding:**
```cpp
for each byte in (preprocessed data):
    predictions = ppm.get_predictions(5 orders)
    probability = mixer.mix(predictions)
    range_encoder.encode_byte(byte, probability)
    ppm.update(byte)  // Update PPM stats (but NOT mixer weights)
```

---

## File Format

```
┌─────────────────────────────────────────────────┐
│                 FILE HEADER                     │
├─────────────────────────────────────────────────┤
│ model_type (1 byte)          = 0x09             │
│ original_size (8 bytes)      = input file size  │
│ processed_size (8 bytes)     = after BWT/MTF    │
│ config_byte (1 byte)         = BWT/MTF/ZRLE     │
│ bwt_primary_index (4 bytes)  = BWT index        │
│ num_orders (1 byte)          = 5                │
│ orders[5] (5 bytes)          = {1,2,3,4,5}      │
│ mixer_weights (40 bytes)     = 5 × int64_t      │
├─────────────────────────────────────────────────┤
│             COMPRESSED DATA                     │
│  (Range-coded bytes using PPM + static mixing)  │
└─────────────────────────────────────────────────┘

Total header: 68 bytes
```

---

## Memory Usage

| Component | Memory | Notes |
|-----------|--------|-------|
| PPM hash table | 768 MB | 8M entries × 96 bytes/entry |
| History buffer | 8 KB | Circular buffer for context |
| Mixer weights | 40 bytes | 5 orders × 8 bytes |
| Range coder state | 16 KB | Internal buffers |
| **Total** | **~768 MB** | Per compression thread |

---

## Performance Characteristics

**Compression speed:** 158 KB/s avg (v5.0: 25 MB/s, 157× faster)

**Bottlenecks:**
1. **Hash table lookups:** 5 lookups per byte (5 orders)
2. **Log/exp approximations:** Stretch/squash tables (1024-entry lookup, fast)
3. **Range coding:** Byte-level division per symbol

**Why so slow vs v5.0?**
- v5.0: 1 order, flat array lookup (O(1) cache-friendly)
- v6.1: 5 orders, hash table lookups (O(1) but cache-unfriendly, 5× overhead)
- v6.1: Context mixing overhead (5 predictions → 1 probability)
- v6.1: Range coder more complex than v5.0's arithmetic coder

---

## Deterministic Compression Guarantee

**Critical requirement:** Encoder and decoder must compute **identical** mixed probabilities.

**How v6.1 achieves this:**
1. **Fixed-point arithmetic:** All computations use integers (no floating-point)
2. **Deterministic rounding:** `std::lround()` ensures consistent double→int conversion
3. **Static weights:** Both sides load identical weights from file header
4. **Lookup tables:** Stretch/squash use 1024-entry tables (shared across platforms)

**Test:** Compress on Linux, decompress on Windows → identical output (verified).

---

## Conclusion

v6.1's architecture is **theoretically sound** but **practically ineffective**:

- **Pro:** Deterministic compression (fixed-point arithmetic)
- **Pro:** Clever sample-based training avoids storing per-byte weight updates
- **Pro:** Reuses proven v5.0 preprocessing (BWT+MTF+ZRLE)

- **Con:** Static weights can't adapt to file sections
- **Con:** Byte-level coding leaves information on the table
- **Con:** 157× slower than v5.0 with worse compression

The fundamental flaw is **static mixing** — PAQ's success comes from **adaptive** weights that improve during compression, not pre-computed fixed weights.

---

*Last updated: 2026-04-06*
