# How It Works: Compression & Decompression

A high-level overview of our lossless data compression system.

---

## The Big Picture

Our compression system combines multiple components that work together:

1. **Model Selection** - Chooses the best probability model based on file characteristics
2. **Probability Model** - Analyzes the data to understand symbol probabilities
   - **Order-0 Frequency Model** - Simple, fast, universal
   - **Order-1 Context Model** - Adaptive, better for low-entropy data
3. **Range Coder** - Encodes the data efficiently based on those probabilities

Together, they achieve compression by representing frequent symbols with fewer bits and rare symbols with more bits.

## Current Version (v2.0)

This document covers the current multi-model implementation. For the simpler v1.0 implementation, see the [v1.0 documentation](../versions/g07_v1.0/README.md).

---

## Part 1: Order-0 Frequency Model

### What It Does

The frequency model **learns the probability distribution** of bytes in the input data.

### How It Works

1. **Count Phase**: Scan through the entire input file and count how many times each byte value (0-255) appears
   - Example: If 'A' appears 100 times and 'Z' appears 5 times, 'A' is more probable

2. **Build Cumulative Frequencies**: Convert counts into ranges for the arithmetic coder
   - Each symbol gets a range proportional to its frequency
   - More frequent symbols get larger ranges

3. **Lookup**: Given a symbol, the model provides:
   - `cum_freq_low`: Start of the symbol's range
   - `cum_freq_high`: End of the symbol's range
   - `total_freq`: Total of all frequencies

### Example

```
Input: "AAABBC"
Frequencies:
  A: 3 times → range [0, 3)
  B: 2 times → range [3, 5)
  C: 1 time  → range [5, 6)
Total: 6
```

### Why Order-0?

"Order-0" means we only look at individual bytes in isolation, not their context. Each byte is treated independently, regardless of what came before it.

---

## Part 2: Range Coding (v2.0)

### What It Does

Range coding **encodes a sequence of symbols into a bitstream** using efficient interval arithmetic. It's a faster variant of arithmetic coding with the same compression efficiency.

### Why Range Coding Instead of Arithmetic?

Range coding (used in v2.0) is faster than traditional arithmetic coding (used in v1.0):
- **Speed**: ~2x faster encoding/decoding
- **Compression**: Same efficiency as arithmetic coding
- **Overhead**: Less than 0.01% file size difference
- **Simplicity**: No carry propagation issues

### How Range Coding Works (Simplified)

Similar to arithmetic coding, but normalizes in byte-sized chunks instead of bit-by-bit:

### How It Works

1. **Start with interval [0, 1]**

2. **For each symbol**:
   - Divide the current interval into sub-intervals proportional to symbol probabilities
   - Narrow to the sub-interval corresponding to the current symbol

3. **Output bits** as the interval narrows (renormalization)

4. **Final output**: A bitstream representing the narrowed interval

### Visual Example

Encoding "AB" where A has 75% probability and B has 25%:

```
Initial interval: [0.0, 1.0)

Encode 'A' (75%):
  [0.0, 1.0) → [0.0, 0.75)

Encode 'B' (25% of remaining):
  [0.0, 0.75) → [0.5625, 0.75)

Final: Any number in [0.5625, 0.75) represents "AB"
```

### Why It's Efficient

- Frequent symbols (large probability) → small interval narrowing → few bits
- Rare symbols (small probability) → large interval narrowing → many bits
- The entire message becomes one fraction, near-optimal compression

---

## Part 2.5: Order-1 Context Model (v2.0)

### What It Does

The Order-1 model uses the **previous byte as context** to predict the next byte, achieving better compression than Order-0 on structured data.

### How It Works

1. **Context-Based Prediction**: Maintains 256 separate frequency tables (one for each possible previous byte)
   - After seeing 'T', 'H' is more probable (English text)
   - After seeing 'Q', 'U' is very probable
   - Context provides better predictions than global frequencies

2. **Adaptive Frequencies**: Updates probabilities during encoding/decoding
   - Starts with uniform distribution
   - Increments frequency after each symbol
   - Both encoder and decoder update identically
   - No need to store model in file header

3. **Escape Mechanism**: Handles unseen symbol/context combinations
   - If symbol not seen in current context, encode ESCAPE
   - Fall back to Order-0 (global frequencies)
   - Ensures all symbols can be encoded

### Example

```
Input: "THE THE"
Context: none → 'T' (Order-0)
Context: 'T'  → 'H' (high probability after 'T')
Context: 'H'  → 'E' (high probability after 'H')
Context: 'E'  → ' ' (space after 'E')
Context: ' '  → 'T' (another 'THE')
...
```

After seeing "THE" once, the model predicts it better the second time!

### Why Order-1?

"Order-1" means using 1 previous byte as context (Order-k uses k bytes).

**Advantages over Order-0**:
- Exploits sequential patterns (20-50% better compression on text)
- Adapts to changing statistics
- No model storage overhead

**Trade-offs**:
- Slower (more table lookups)
- Higher memory (256 contexts)
- Less effective on random/high-entropy data

---

## Part 3: Compression Flow

### Step-by-Step Process

1. **Read Input File**
   ```
   data = read_file("input.txt")
   ```

2. **Build Frequency Model**
   ```
   model.build_from_data(data)
   // Counts all bytes, builds cumulative frequency table
   ```

3. **Encode Each Symbol**
   ```
   for each byte in data:
       (low, high) = model.get_symbol_range(byte)
       encoder.encode_symbol(low, high, model.total_freq)
   ```

4. **Encode EOF Symbol**
   ```
   (low, high) = model.get_symbol_range(EOF)
   encoder.encode_symbol(low, high, model.total_freq)
   ```

5. **Write Compressed File**
   ```
   output = [original_size] + [frequency_table] + [encoded_bits]
   write_file("output.compressed", output)
   ```

### Compressed File Format

```
+------------------+-------------------------+------------------+
| Original Size    | Frequency Table         | Encoded Data     |
| (8 bytes)        | (257 × 4 = 1028 bytes) | (variable)       |
+------------------+-------------------------+------------------+
```

**Why save the frequency table?**
The decompressor needs the exact same probability model to decode correctly.

---

## Part 4: Decompression Flow

### Step-by-Step Process

1. **Read Compressed File**
   ```
   compressed = read_file("output.compressed")
   ```

2. **Extract Header Information**
   ```
   original_size = compressed[0:8]
   frequency_table = compressed[8:1036]
   encoded_data = compressed[1036:]
   ```

3. **Rebuild Frequency Model**
   ```
   model.set_frequencies(frequency_table)
   // Reconstructs the same model used during compression
   ```

4. **Decode Symbols**
   ```
   decoder = ArithmeticDecoder(encoded_data)

   while output.size < original_size:
       current_value = decoder.get_current_count(model.total_freq)
       symbol = model.find_symbol(current_value)

       if symbol == EOF:
           break

       output.append(symbol)

       (low, high) = model.get_symbol_range(symbol)
       decoder.decode_symbol(low, high, model.total_freq)
   ```

5. **Write Decompressed File**
   ```
   write_file("output.txt", output)
   ```

### How Decoding Works

The decoder:
1. Reads bits from the encoded data to maintain a "current value"
2. Asks the model: "Which symbol has a range containing this value?"
3. Updates its interval based on the symbol's range (same logic as encoder)
4. Repeats until all symbols are decoded

---

## Why This Works Together

### Symmetry

The encoder and decoder use the **exact same frequency model** and follow **mirror operations**:

- **Encoder**: Symbol → Range → Narrow interval → Output bits
- **Decoder**: Input bits → Current value → Find range → Symbol

### Lossless Guarantee

Because:
1. The frequency model is saved in the file header (no information loss)
2. Arithmetic coding is reversible (mathematical guarantee)
3. EOF symbol marks the end unambiguously

The decompressor can **perfectly reconstruct** the original data byte-for-byte.

---

## Performance Characteristics

### Strengths

- **Near-optimal compression** for the given probability model
- **Simple and elegant** - Only needs byte frequencies
- **Works well** on data with skewed distributions (some bytes much more common)

### Limitations

- **Order-0 limitation**: Doesn't exploit patterns (e.g., "TH" after "THE")
- **Header overhead**: 1036 bytes fixed cost (affects small files)
- **Static model**: Frequencies computed once, not adapted during encoding

### When It Excels

- Text with repeated characters
- Data with non-uniform byte distributions
- Files where simple frequency analysis captures the structure

---

## Part 5: Model Selection (v2.0 Auto Mode)

### What It Does

The auto-selection algorithm chooses the best model for each file based on its characteristics.

### How It Works

1. **Calculate Entropy**: Sample the file to estimate entropy (bits/symbol)
   ```cpp
   entropy = calculate_entropy(input_data, sample_size=8192)
   ```

2. **Decision Logic**:
   ```
   if (entropy > 7.5):
       → UNCOMPRESSED (incompressible, avoid expansion)
   elif (entropy > 6.8):
       → ORDER-0 (high entropy, Order-1 too slow for marginal gain)
   elif (file_size < 100KB):
       → ORDER-0 (adaptive overhead not worthwhile)
   else:
       → ORDER-1 (low entropy, good patterns expected)
   ```

3. **Store Model Type**: Write model ID to file header (1 byte)

### Why This Works

| File Type | Entropy | Model Chosen | Reason |
|-----------|---------|--------------|--------|
| Random data (File D) | ~8.0 | Uncompressed | Can't compress, avoid overhead |
| Binary executables | ~7.0-7.5 | Order-0 | Fast, reasonable compression |
| English text | ~4.5-5.5 | Order-1 | Patterns = big win |
| Structured (XML/JSON) | ~3.0-5.0 | Order-1 | Repetition = excellent compression |

### Performance

From benchmarks:
- **Auto-selection success rate**: ~90% (chooses optimal or near-optimal model)
- **Files won**: Competitive across all file types
- **User benefit**: No need to manually choose model

---

## Summary

### Version 2.0 Flow

```
COMPRESSION:
  Input File → Analyze Entropy → Select Model → Encode with Range Coding → Compressed File
             ↓
       Order-0 (fast) or Order-1 (better compression) or Uncompressed

DECOMPRESSION:
  Compressed File → Read Model Type → Build/Load Model → Decode with Range Coding → Output File
```

### Version 1.0 Flow (For Comparison)

```
COMPRESSION:
  Input File → Count Frequencies → Build Order-0 Model → Encode with Arithmetic Coding → Compressed File

DECOMPRESSION:
  Compressed File → Extract Model → Decode with Arithmetic Coding → Output File
```

**Key Insights**:
- v2.0 adds **intelligent model selection** for optimal compression
- **Range coding** provides same compression as arithmetic with 2x speed
- **Order-1 adaptive model** exploits patterns for 20-50% better compression on structured data
- **Auto-selection** achieves near-optimal results across diverse file types

---

**Implementation**:
- Range coder: `src/arithmetic/range_coder.cpp`
- Order-0 model: `src/model/frequency_model.cpp`
- Order-1 model: `src/model/context_model.cpp`
- Auto-selection: `src/compressor.cpp`
