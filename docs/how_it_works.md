# How It Works: Compression & Decompression

A high-level overview of our lossless data compression system.

---

## The Big Picture

Our compression system has two main components that work together:

1. **Order-0 Frequency Model** - Analyzes the data to understand symbol probabilities
2. **Arithmetic Coder** - Encodes the data efficiently based on those probabilities

Together, they achieve compression by representing frequent symbols with fewer bits and rare symbols with more bits.

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

## Part 2: Arithmetic Coding

### What It Does

Arithmetic coding **encodes a sequence of symbols into a single number** between 0 and 1, represented as a bitstream.

### How It Works

1. **Start with interval [0, 1)**

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

## Summary

```
COMPRESSION:
  Input File → Count Frequencies → Build Model → Encode with Arithmetic Coding → Compressed File

DECOMPRESSION:
  Compressed File → Extract Model → Decode with Arithmetic Coding → Output File
```

**Key Insight**: By combining statistical modeling (frequency analysis) with efficient encoding (arithmetic coding), we achieve compression rates that approach the theoretical entropy limit for order-0 models.

---

**Implementation**: `src/model/frequency_model.cpp`, `src/arithmetic/arithmetic_coder.cpp`
