# Model Comparison Benchmark Results

**Date**: $(date)
**Range Coder**: Schindler's Byte-Normalized Range Coder

---

## Performance Comparison


### Compression Ratio Comparison

| File | Original | Order-0 | Order-1 | Order-2 | Best |
|------|----------|---------|---------|---------|------|
| A | 1.3MiB | 52.3053% | 52.0959% | 52.0959% | Order-1 |
| B | 1.2MiB | 67.5639% | 32.6082% | 32.6082% | Order-1 |
| C | 2.0MiB | 66.1761% | 40.1228% | 40.1228% | Order-1 |
| D | 2.0MiB | 100.1% | 125.078% | 125.078% | Order-0 |
| E | 989KiB | 88.4138% | 93.4923% | 93.4923% | Order-0 |
| F | 2.0MiB | 88.0383% | 88.0018% | 88.0018% | Order-1 |
| G | 2.5MiB | 41.9197% | 27.4389% | 27.4389% | Order-1 |
| H | 999KiB | 80.8507% | 50.8119% | 50.8119% | Order-1 |

### Compression Speed Comparison

| File | Order-0 (ms) | Order-1 (ms) | Order-2 (ms) | Fastest |
|------|--------------|--------------|--------------|---------|
| A | 8 | 549 | 551 | TBD |
| B | 10 | 718 | 715 | TBD |
| C | 14 | 1408 | 1426 | TBD |
| D | 17 | 33910 | 29800 | TBD |
| E | 8 | 18575 | 21407 | TBD |
| F | 16 | 27488 | 26932 | TBD |
| G | 17 | 1163 | 1172 | TBD |
| H | 10 | 3695 | 3619 | TBD |

### Decompression Speed Comparison

| File | Order-0 (ms) | Order-1 (ms) | Order-2 (ms) | Fastest |
|------|--------------|--------------|--------------|---------|
| A | 34 | TIMEOUT | TIMEOUT | TBD |
| B | 27 | TIMEOUT | TIMEOUT | TBD |
| C | 41 | TIMEOUT | TIMEOUT | TBD |
| D | 80 | TIMEOUT | TIMEOUT | TBD |
| E | 36 | TIMEOUT | TIMEOUT | TBD |
| F | 71 | TIMEOUT | TIMEOUT | TBD |
| G | 48 | TIMEOUT | TIMEOUT | TBD |
| H | 29 | TIMEOUT | TIMEOUT | TBD |
