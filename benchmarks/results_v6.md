# v6.0 Compression Benchmark Results

**Date:** Mon Apr  6 05:47:39 PM WEST 2026

**Configuration:** Multi-Order PPM (Orders: 1,2,4,6,8,12,16,24,32)

| File | Original Size | Compressed Size | Ratio | Bits/Symbol | Compress Time | Decompress Time | Throughput |
|------|---------------|-----------------|-------|-------------|---------------|-----------------|------------|
| A | 1.3MiB | 775KiB | 60.61% | 4.84 | 7639ms | 7183ms | 167.50 KB/s |
| B | 1.2MiB | 222KiB | 19.39% | 1.55 | 1617ms | 1458ms | 708.92 KB/s |
| C | 2.0MiB | 564KiB | 28.84% | 2.30 | 5170ms | 4853ms | 377.77 KB/s |
| D | 2.0MiB | 2.0MiB | 100.00% | 8.00 | 22857ms | 22211ms | 85.47 KB/s |
| E | 989KiB | 978KiB | 98.86% | 7.90 | 8998ms | 8735ms | 109.94 KB/s |
| F | 2.0MiB | 1.7MiB | 84.38% | 6.75 | 20399ms | 20048ms | 100.44 KB/s |
| G | 2.5MiB | 875KiB | 35.43% | 2.83 | 10063ms | 9357ms | 245.28 KB/s |
| H | 999KiB | 587KiB | 58.70% | 4.69 | 4376ms | 4109ms | 228.55 KB/s |

## Summary

- **Files tested:** 8
- **Total original size:** 13MiB
- **Total compressed size:** 7.5MiB
- **Average compression ratio:** 59.85%
- **Average bits/symbol:** 4.78
- **Average compression time:** 10139.87ms
- **Average decompression time:** 9744.25ms
- **Overall throughput:** 158.16 KB/s
